#include "speck.h"
#include <stdexcept>
#include <cstring>

static uint64_t rotr64(uint64_t x, int r) {
    return (x >> r) | (x << (64 - r));
}

static uint64_t rotl64(uint64_t x, int r) {
    return (x << r) | (x >> (64 - r));
}

static void packBlock(uint64_t x, uint64_t y, uint8_t* out) {
    for (int i = 0; i < 8; ++i) out[i] = (x >> (8 * i)) & 0xFF;
    for (int i = 0; i < 8; ++i) out[i + 8] = (y >> (8 * i)) & 0xFF;
}

static void unpackBlock(const uint8_t* in, uint64_t& x, uint64_t& y) {
    x = y = 0;
    for (int i = 0; i < 8; ++i) x |= (static_cast<uint64_t>(in[i]) << (8 * i));
    for (int i = 0; i < 8; ++i) y |= (static_cast<uint64_t>(in[i + 8]) << (8 * i));
}

static std::vector<uint8_t> pkcs7Pad(const std::vector<uint8_t>& data) {
    uint8_t padLen = static_cast<uint8_t>(SPECK_BLOCK_BYTES - (data.size() % SPECK_BLOCK_BYTES));
    std::vector<uint8_t> padded(data);
    padded.insert(padded.end(), padLen, padLen);
    return padded;
}

static std::vector<uint8_t> pkcs7Unpad(const std::vector<uint8_t>& data) {
    if (data.empty() || data.size() % SPECK_BLOCK_BYTES != 0)
        throw std::runtime_error("Padding error");

    uint8_t padLen = data.back();
    if (padLen == 0 || padLen > SPECK_BLOCK_BYTES)
        throw std::runtime_error("Padding error");

    for (size_t i = data.size() - padLen; i < data.size(); i++) {
        if (data[i] != padLen)
            throw std::runtime_error("Padding error");
    }
    return std::vector<uint8_t>(data.begin(), data.end() - padLen);
}

void speckInitSchedule(SpeckKeySchedule& sched) {
    std::memset(sched.K, 0, sizeof(sched.K));
    sched.rounds = 0;
    sched.keyIsSet = false;
}

bool speckSetKey(SpeckKeySchedule& sched, const std::vector<uint8_t>& key) {
    if (key.size() != 16 && key.size() != 24 && key.size() != 32)
        return false;

    int m = key.size() / 8;
    sched.rounds = (m == 2) ? 32 : ((m == 3) ? 33 : 34);

    uint64_t L[3] = {0};
    for (int i = 0; i < m - 1; i++) {
        for (int j = 0; j < 8; j++) L[i] |= (static_cast<uint64_t>(key[(i + 1) * 8 + j]) << (8 * j));
    }

    sched.K[0] = 0;
    for (int j = 0; j < 8; j++) sched.K[0] |= (static_cast<uint64_t>(key[j]) << (8 * j));

    for (int i = 0; i < sched.rounds - 1; i++) {
        uint64_t l_i = L[i % (m - 1)];
        uint64_t new_l = (sched.K[i] + rotr64(l_i, 8)) ^ i;
        sched.K[i + 1] = rotl64(sched.K[i], 3) ^ new_l;
        L[i % (m - 1)] = new_l;
    }

    sched.keyIsSet = true;
    return true;
}

void speckEncryptBlock(const SpeckKeySchedule& sched, uint64_t& x, uint64_t& y) {
    for (int i = 0; i < sched.rounds; i++) {
        x = (rotr64(x, 8) + y) ^ sched.K[i];
        y = rotl64(y, 3) ^ x;
    }
}

void speckDecryptBlock(const SpeckKeySchedule& sched, uint64_t& x, uint64_t& y) {
    for (int i = sched.rounds - 1; i >= 0; i--) {
        y = rotr64(y ^ x, 3);
        x = rotl64((x ^ sched.K[i]) - y, 8);
    }
}

std::vector<uint8_t> speckEncryptCBC(const SpeckKeySchedule& sched,
                                     const std::vector<uint8_t>& plaintext,
                                     const std::vector<uint8_t>& iv) {
    if (!sched.keyIsSet || iv.size() != SPECK_BLOCK_BYTES)
        throw std::runtime_error("Invalid state");

    std::vector<uint8_t> padded = pkcs7Pad(plaintext);
    std::vector<uint8_t> cipher(padded.size());

    uint8_t prev[SPECK_BLOCK_BYTES];
    std::memcpy(prev, iv.data(), SPECK_BLOCK_BYTES);

    for (size_t offset = 0; offset < padded.size(); offset += SPECK_BLOCK_BYTES) {
        uint8_t block[SPECK_BLOCK_BYTES];
        for (int b = 0; b < SPECK_BLOCK_BYTES; b++)
            block[b] = padded[offset + b] ^ prev[b];

        uint64_t x, y;
        unpackBlock(block, x, y);
        speckEncryptBlock(sched, x, y);
        packBlock(x, y, &cipher[offset]);

        std::memcpy(prev, &cipher[offset], SPECK_BLOCK_BYTES);
    }
    return cipher;
}

std::vector<uint8_t> speckDecryptCBC(const SpeckKeySchedule& sched,
                                     const std::vector<uint8_t>& ciphertext,
                                     const std::vector<uint8_t>& iv) {
    if (!sched.keyIsSet || iv.size() != SPECK_BLOCK_BYTES || ciphertext.empty() || ciphertext.size() % SPECK_BLOCK_BYTES != 0)
        throw std::runtime_error("Invalid state");

    std::vector<uint8_t> padded(ciphertext.size());
    uint8_t prev[SPECK_BLOCK_BYTES];
    std::memcpy(prev, iv.data(), SPECK_BLOCK_BYTES);

    for (size_t offset = 0; offset < ciphertext.size(); offset += SPECK_BLOCK_BYTES) {
        uint64_t x, y;
        unpackBlock(&ciphertext[offset], x, y);
        speckDecryptBlock(sched, x, y);

        uint8_t block[SPECK_BLOCK_BYTES];
        packBlock(x, y, block);

        for (int b = 0; b < SPECK_BLOCK_BYTES; b++)
            padded[offset + b] = block[b] ^ prev[b];

        std::memcpy(prev, &ciphertext[offset], SPECK_BLOCK_BYTES);
    }
    return pkcs7Unpad(padded);
}