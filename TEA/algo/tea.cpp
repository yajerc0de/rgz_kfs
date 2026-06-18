#include "tea.h"

#include <stdexcept>
#include <cstring>

using namespace std;

// =============================================================================
//  Конструктор
// =============================================================================

TEA::TEA() {
    memset(m_key, 0, sizeof(m_key));
}

// =============================================================================
//  setKey — загрузка ключа
//
//  Ключ 16 байт разбивается на 4 слова uint32_t в big-endian порядке:
//  key[0] = байты 0–3, key[1] = байты 4–7, key[2] = байты 8–11, key[3] = байты 12–15
// =============================================================================

bool TEA::setKey(const vector<uint8_t>& key) {
    if (key.size() != KEY_BYTES)
        return false;

    for (int i = 0; i < KEY_WORDS; i++) {
        m_key[i] = (uint32_t(key[i * 4    ]) << 24)
                 | (uint32_t(key[i * 4 + 1]) << 16)
                 | (uint32_t(key[i * 4 + 2]) <<  8)
                 |  uint32_t(key[i * 4 + 3]);
    }

    m_keyIsSet = true;
    return true;
}

// =============================================================================
//  Вспомогательные функции — упаковка/распаковка блока (big-endian)
// =============================================================================

void TEA::packBlock(uint32_t v0, uint32_t v1, uint8_t* out) {
    out[0] = (v0 >> 24) & 0xFF; out[1] = (v0 >> 16) & 0xFF;
    out[2] = (v0 >>  8) & 0xFF; out[3] = (v0      ) & 0xFF;
    out[4] = (v1 >> 24) & 0xFF; out[5] = (v1 >> 16) & 0xFF;
    out[6] = (v1 >>  8) & 0xFF; out[7] = (v1      ) & 0xFF;
}

void TEA::unpackBlock(const uint8_t* in, uint32_t& v0, uint32_t& v1) {
    v0 = (uint32_t(in[0]) << 24) | (uint32_t(in[1]) << 16)
       | (uint32_t(in[2]) <<  8) |  uint32_t(in[3]);
    v1 = (uint32_t(in[4]) << 24) | (uint32_t(in[5]) << 16)
       | (uint32_t(in[6]) <<  8) |  uint32_t(in[7]);
}

// =============================================================================
//  encryptBlock — шифрование одного 64-битного блока
//
//  32 цикла, каждый цикл обновляет оба слова через DELTA:
//    sum += DELTA
//    v0  += ((v1 << 4) + key[0]) XOR (v1 + sum) XOR ((v1 >> 5) + key[1])
//    v1  += ((v0 << 4) + key[2]) XOR (v0 + sum) XOR ((v0 >> 5) + key[3])
// =============================================================================

void TEA::encryptBlock(uint32_t& v0, uint32_t& v1) const {
    uint32_t sum = 0;

    for (int i = 0; i < ROUNDS; i++) {
        sum += DELTA;
        v0  += ((v1 << 4) + m_key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + m_key[1]);
        v1  += ((v0 << 4) + m_key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + m_key[3]);
    }
}

// =============================================================================
//  decryptBlock — дешифрование одного 64-битного блока
//
//  Зеркально: sum начинается с DELTA * ROUNDS и убывает,
//  операции v1 и v0 применяются в обратном порядке.
// =============================================================================

void TEA::decryptBlock(uint32_t& v0, uint32_t& v1) const {
    uint32_t sum = DELTA * static_cast<uint32_t>(ROUNDS);

    for (int i = 0; i < ROUNDS; i++) {
        v1  -= ((v0 << 4) + m_key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + m_key[3]);
        v0  -= ((v1 << 4) + m_key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + m_key[1]);
        sum -= DELTA;
    }
}

// =============================================================================
//  PKCS#7 паддинг
// =============================================================================

vector<uint8_t> TEA::pkcs7Pad(const vector<uint8_t>& data) {
    uint8_t padLen = BLOCK_BYTES - (data.size() % BLOCK_BYTES);
    vector<uint8_t> padded(data);
    padded.insert(padded.end(), padLen, padLen);
    return padded;
}

vector<uint8_t> TEA::pkcs7Unpad(const vector<uint8_t>& data) {
    if (data.empty() || data.size() % BLOCK_BYTES != 0)
        throw runtime_error("TEA: неверный размер данных при снятии паддинга");

    uint8_t padLen = data.back();
    if (padLen == 0 || padLen > BLOCK_BYTES)
        throw runtime_error("TEA: повреждён PKCS#7 паддинг");

    for (size_t i = data.size() - padLen; i < data.size(); i++) {
        if (data[i] != padLen)
            throw runtime_error("TEA: неверный PKCS#7 паддинг (возможно, неверный ключ)");
    }
    return vector<uint8_t>(data.begin(), data.end() - padLen);
}

// =============================================================================
//  CBC-режим
// =============================================================================

vector<uint8_t> TEA::encryptCBC(const vector<uint8_t>& plaintext,
                                 const vector<uint8_t>& iv) const {
    if (!m_keyIsSet)
        throw runtime_error("TEA: ключ не установлен");
    if (iv.size() != BLOCK_BYTES)
        throw runtime_error("TEA: IV должен быть ровно 8 байт");

    vector<uint8_t> padded = pkcs7Pad(plaintext);
    vector<uint8_t> cipher(padded.size());

    uint8_t prev[BLOCK_BYTES];
    memcpy(prev, iv.data(), BLOCK_BYTES);

    for (size_t offset = 0; offset < padded.size(); offset += BLOCK_BYTES) {
        // XOR с предыдущим шифроблоком (или IV)
        uint8_t block[BLOCK_BYTES];
        for (int b = 0; b < BLOCK_BYTES; b++)
            block[b] = padded[offset + b] ^ prev[b];

        uint32_t v0, v1;
        unpackBlock(block, v0, v1);
        encryptBlock(v0, v1);
        packBlock(v0, v1, &cipher[offset]);

        memcpy(prev, &cipher[offset], BLOCK_BYTES);
    }
    return cipher;
}

vector<uint8_t> TEA::decryptCBC(const vector<uint8_t>& ciphertext,
                                 const vector<uint8_t>& iv) const {
    if (!m_keyIsSet)
        throw runtime_error("TEA: ключ не установлен");
    if (iv.size() != BLOCK_BYTES)
        throw runtime_error("TEA: IV должен быть ровно 8 байт");
    if (ciphertext.empty() || ciphertext.size() % BLOCK_BYTES != 0)
        throw runtime_error("TEA: размер шифротекста не кратен 8 байтам");

    vector<uint8_t> padded(ciphertext.size());

    uint8_t prev[BLOCK_BYTES];
    memcpy(prev, iv.data(), BLOCK_BYTES);

    for (size_t offset = 0; offset < ciphertext.size(); offset += BLOCK_BYTES) {
        uint32_t v0, v1;
        unpackBlock(&ciphertext[offset], v0, v1);
        decryptBlock(v0, v1); 

        uint8_t block[BLOCK_BYTES];
        packBlock(v0, v1, block);

        for (int b = 0; b < BLOCK_BYTES; b++)
            padded[offset + b] = block[b] ^ prev[b];

        memcpy(prev, &ciphertext[offset], BLOCK_BYTES);
    }
    return pkcs7Unpad(padded);
}