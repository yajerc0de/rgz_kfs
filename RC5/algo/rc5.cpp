#include "rc5.h"

#include <stdexcept>
#include <cstring>

using namespace std;



RC5::RC5() {
    memset(m_S, 0, sizeof(m_S));
}



uint32_t RC5::rotl(uint32_t x, uint32_t s) {
    s &= (W - 1);
    if (s == 0) return x;
    return (x << s) | (x >> (W - s));
}

uint32_t RC5::rotr(uint32_t x, uint32_t s) {
    s &= (W - 1);
    if (s == 0) return x;
    return (x >> s) | (x << (W - s));
}



void RC5::packBlock(uint32_t A, uint32_t B, uint8_t* out) {
    out[0] = (A      ) & 0xFF; out[1] = (A >>  8) & 0xFF;
    out[2] = (A >> 16) & 0xFF; out[3] = (A >> 24) & 0xFF;
    out[4] = (B      ) & 0xFF; out[5] = (B >>  8) & 0xFF;
    out[6] = (B >> 16) & 0xFF; out[7] = (B >> 24) & 0xFF;
}

void RC5::unpackBlock(const uint8_t* in, uint32_t& A, uint32_t& B) {
    A =  uint32_t(in[0])
      | (uint32_t(in[1]) <<  8)
      | (uint32_t(in[2]) << 16)
      | (uint32_t(in[3]) << 24);
    B =  uint32_t(in[4])
      | (uint32_t(in[5]) <<  8)
      | (uint32_t(in[6]) << 16)
      | (uint32_t(in[7]) << 24);
}



void RC5::expandKey(const vector<uint8_t>& key) {
    const int b = static_cast<int>(key.size()); 
    const int u = W / 8;                        
    const int c = (b + u - 1) / u;              

    
    vector<uint32_t> L(c, 0);
    for (int i = b - 1; i >= 0; i--)
        L[i / u] = (L[i / u] << 8) + key[i];

   
    m_S[0] = P32;
    for (int i = 1; i < TABLE_SIZE; i++)
        m_S[i] = m_S[i - 1] + Q32;

  
    uint32_t A = 0, B = 0;
    int i = 0, j = 0;
    int iterations = 3 * max(TABLE_SIZE, c);

    for (int k = 0; k < iterations; k++) {
        m_S[i] = rotl(m_S[i] + A + B, 3);
        A = m_S[i];
        i = (i + 1) % TABLE_SIZE;

        L[j] = rotl(L[j] + A + B, (A + B) & (W - 1));
        B = L[j];
        j = (j + 1) % c;
    }
}



bool RC5::setKey(const vector<uint8_t>& key) {
    if (key.empty() || key.size() > 255)
        return false;

    expandKey(key);
    m_keyIsSet = true;
    return true;
}



void RC5::encryptBlock(uint32_t& A, uint32_t& B) const {
    A = A + m_S[0];
    B = B + m_S[1];

    for (int i = 1; i <= ROUNDS; i++) {
        A = rotl(A ^ B, B) + m_S[2 * i];
        B = rotl(B ^ A, A) + m_S[2 * i + 1];
    }
}



void RC5::decryptBlock(uint32_t& A, uint32_t& B) const {
    for (int i = ROUNDS; i >= 1; i--) {
        B = rotr(B - m_S[2 * i + 1], A) ^ A;
        A = rotr(A - m_S[2 * i],     B) ^ B;
    }

    B = B - m_S[1];
    A = A - m_S[0];
}



vector<uint8_t> RC5::pkcs7Pad(const vector<uint8_t>& data) {
    uint8_t padLen = static_cast<uint8_t>(BLOCK_BYTES - (data.size() % BLOCK_BYTES));
    vector<uint8_t> padded(data);
    padded.insert(padded.end(), padLen, padLen);
    return padded;
}

vector<uint8_t> RC5::pkcs7Unpad(const vector<uint8_t>& data) {
    if (data.empty() || data.size() % BLOCK_BYTES != 0)
        throw runtime_error("RC5: неверный размер данных при снятии паддинга");

    uint8_t padLen = data.back();
    if (padLen == 0 || padLen > BLOCK_BYTES)
        throw runtime_error("RC5: повреждён PKCS#7 паддинг");

    for (size_t i = data.size() - padLen; i < data.size(); i++) {
        if (data[i] != padLen)
            throw runtime_error("RC5: неверный PKCS#7 паддинг (возможно, неверный ключ)");
    }
    return vector<uint8_t>(data.begin(), data.end() - padLen);
}



vector<uint8_t> RC5::encryptCBC(const vector<uint8_t>& plaintext,
                                 const vector<uint8_t>& iv) const {
    if (!m_keyIsSet)
        throw runtime_error("RC5: ключ не установлен");
    if (iv.size() != BLOCK_BYTES)
        throw runtime_error("RC5: IV должен быть ровно 8 байт");

    vector<uint8_t> padded = pkcs7Pad(plaintext);
    vector<uint8_t> cipher(padded.size());

    uint8_t prev[BLOCK_BYTES];
    memcpy(prev, iv.data(), BLOCK_BYTES);

    for (size_t offset = 0; offset < padded.size(); offset += BLOCK_BYTES) {
        // XOR с предыдущим шифроблоком (или IV)
        uint8_t block[BLOCK_BYTES];
        for (int b = 0; b < BLOCK_BYTES; b++)
            block[b] = padded[offset + b] ^ prev[b];

        uint32_t A, B;
        unpackBlock(block, A, B);
        encryptBlock(A, B);
        packBlock(A, B, &cipher[offset]);

        memcpy(prev, &cipher[offset], BLOCK_BYTES);
    }
    return cipher;
}

vector<uint8_t> RC5::decryptCBC(const vector<uint8_t>& ciphertext,
                                 const vector<uint8_t>& iv) const {
    if (!m_keyIsSet)
        throw runtime_error("RC5: ключ не установлен");
    if (iv.size() != BLOCK_BYTES)
        throw runtime_error("RC5: IV должен быть ровно 8 байт");
    if (ciphertext.empty() || ciphertext.size() % BLOCK_BYTES != 0)
        throw runtime_error("RC5: размер шифротекста не кратен 8 байтам");

    vector<uint8_t> padded(ciphertext.size());

    uint8_t prev[BLOCK_BYTES];
    memcpy(prev, iv.data(), BLOCK_BYTES);

    for (size_t offset = 0; offset < ciphertext.size(); offset += BLOCK_BYTES) {
        uint32_t A, B;
        unpackBlock(&ciphertext[offset], A, B);
        decryptBlock(A, B);

        uint8_t block[BLOCK_BYTES];
        packBlock(A, B, block);

        for (int b = 0; b < BLOCK_BYTES; b++)
            padded[offset + b] = block[b] ^ prev[b];

        memcpy(prev, &ciphertext[offset], BLOCK_BYTES);
    }
    return pkcs7Unpad(padded);
}
