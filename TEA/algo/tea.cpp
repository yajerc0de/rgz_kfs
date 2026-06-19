#include "tea.h"

#include <cstring>

// =============================================================================
//  Вспомогательные функции — упаковка/распаковка блока (big-endian)
//  static: видны только внутри этого .cpp
// =============================================================================

static void pack_block(uint32_t v0, uint32_t v1, uint8_t* out) {
    out[0] = (v0 >> 24) & 0xFF; out[1] = (v0 >> 16) & 0xFF;
    out[2] = (v0 >>  8) & 0xFF; out[3] = (v0      ) & 0xFF;
    out[4] = (v1 >> 24) & 0xFF; out[5] = (v1 >> 16) & 0xFF;
    out[6] = (v1 >>  8) & 0xFF; out[7] = (v1      ) & 0xFF;
}

static void unpack_block(const uint8_t* in, uint32_t* v0, uint32_t* v1) {
    *v0 = (uint32_t(in[0]) << 24) | (uint32_t(in[1]) << 16)
        | (uint32_t(in[2]) <<  8) |  uint32_t(in[3]);
    *v1 = (uint32_t(in[4]) << 24) | (uint32_t(in[5]) << 16)
        | (uint32_t(in[6]) <<  8) |  uint32_t(in[7]);
}

// =============================================================================
//  PKCS#7 паддинг
//  static: видны только внутри этого .cpp
// =============================================================================

static std::vector<uint8_t> pkcs7_pad(const std::vector<uint8_t>& data) {
    uint8_t pad_len = (uint8_t)(TEA_BLOCK_BYTES_ALGO - (data.size() % TEA_BLOCK_BYTES_ALGO));
    std::vector<uint8_t> padded(data);
    padded.insert(padded.end(), pad_len, pad_len);
    return padded;
}

// Возвращает данные без паддинга или пустой вектор при ошибке.
static std::vector<uint8_t> pkcs7_unpad(const std::vector<uint8_t>& data) {
    if (data.empty() || data.size() % TEA_BLOCK_BYTES_ALGO != 0)
        return {};

    uint8_t pad_len = data.back();
    if (pad_len == 0 || pad_len > TEA_BLOCK_BYTES_ALGO)
        return {};

    for (size_t i = data.size() - pad_len; i < data.size(); ++i) {
        if (data[i] != pad_len)
            return {};
    }

    return std::vector<uint8_t>(data.begin(), data.end() - pad_len);
}

// =============================================================================
//  tea_key_init — сбросить структуру ключа
// =============================================================================

void tea_key_init(TeaKey* tk) {
    if (tk == nullptr) return;
    memset(tk->k, 0, sizeof(tk->k));
    tk->ready = false;
}

// =============================================================================
//  tea_key_set — загрузить ключ (16 байт, big-endian)
//
//  key[0] = байты 0–3, key[1] = байты 4–7, key[2] = байты 8–11, key[3] = байты 12–15
// =============================================================================

bool tea_key_set(TeaKey* tk, const uint8_t* key, int keyLen) {
    if (tk == nullptr || key == nullptr) return false;
    if (keyLen != TEA_KEY_BYTES)         return false;

    for (int i = 0; i < TEA_KEY_WORDS; ++i) {
        tk->k[i] = (uint32_t(key[i * 4    ]) << 24)
                 | (uint32_t(key[i * 4 + 1]) << 16)
                 | (uint32_t(key[i * 4 + 2]) <<  8)
                 |  uint32_t(key[i * 4 + 3]);
    }

    tk->ready = true;
    return true;
}

// =============================================================================
//  tea_encrypt_block — шифрование одного 64-битного блока
//
//  32 цикла, каждый цикл обновляет оба слова через DELTA:
//    sum += DELTA
//    v0  += ((v1 << 4) + key[0]) XOR (v1 + sum) XOR ((v1 >> 5) + key[1])
//    v1  += ((v0 << 4) + key[2]) XOR (v0 + sum) XOR ((v0 >> 5) + key[3])
// =============================================================================

void tea_encrypt_block(const TeaKey* tk, uint32_t* v0, uint32_t* v1) {
    uint32_t sum = 0;

    for (int i = 0; i < TEA_ROUNDS; ++i) {
        sum += TEA_DELTA;
        *v0 += ((*v1 << 4) + tk->k[0]) ^ (*v1 + sum) ^ ((*v1 >> 5) + tk->k[1]);
        *v1 += ((*v0 << 4) + tk->k[2]) ^ (*v0 + sum) ^ ((*v0 >> 5) + tk->k[3]);
    }
}

// =============================================================================
//  tea_decrypt_block — дешифрование одного 64-битного блока
//
//  Зеркально: sum начинается с DELTA * ROUNDS и убывает,
//  операции v1 и v0 применяются в обратном порядке.
// =============================================================================

void tea_decrypt_block(const TeaKey* tk, uint32_t* v0, uint32_t* v1) {
    uint32_t sum = TEA_DELTA * (uint32_t)TEA_ROUNDS;

    for (int i = 0; i < TEA_ROUNDS; ++i) {
        *v1 -= ((*v0 << 4) + tk->k[2]) ^ (*v0 + sum) ^ ((*v0 >> 5) + tk->k[3]);
        *v0 -= ((*v1 << 4) + tk->k[0]) ^ (*v1 + sum) ^ ((*v1 >> 5) + tk->k[1]);
        sum -= TEA_DELTA;
    }
}

// =============================================================================
//  tea_cbc_encrypt — шифрование в режиме CBC + PKCS#7 паддинг
// =============================================================================

std::vector<uint8_t> tea_cbc_encrypt(
    const TeaKey*               tk,
    const std::vector<uint8_t>& plaintext,
    const uint8_t*              iv)
{
    if (tk == nullptr || !tk->ready || iv == nullptr)
        return {};

    std::vector<uint8_t> padded = pkcs7_pad(plaintext);
    std::vector<uint8_t> cipher(padded.size());

    uint8_t prev[TEA_BLOCK_BYTES_ALGO];
    memcpy(prev, iv, TEA_BLOCK_BYTES_ALGO);

    for (size_t offset = 0; offset < padded.size(); offset += TEA_BLOCK_BYTES_ALGO) {
        // XOR с предыдущим шифроблоком (или IV)
        uint8_t block[TEA_BLOCK_BYTES_ALGO];
        for (int b = 0; b < TEA_BLOCK_BYTES_ALGO; ++b)
            block[b] = padded[offset + b] ^ prev[b];

        uint32_t v0, v1;
        unpack_block(block, &v0, &v1);
        tea_encrypt_block(tk, &v0, &v1);
        pack_block(v0, v1, &cipher[offset]);

        memcpy(prev, &cipher[offset], TEA_BLOCK_BYTES_ALGO);
    }

    return cipher;
}

// =============================================================================
//  tea_cbc_decrypt — дешифрование в режиме CBC, снятие PKCS#7 паддинга
// =============================================================================

std::vector<uint8_t> tea_cbc_decrypt(
    const TeaKey*               tk,
    const std::vector<uint8_t>& ciphertext,
    const uint8_t*              iv)
{
    if (tk == nullptr || !tk->ready || iv == nullptr)
        return {};
    if (ciphertext.empty() || ciphertext.size() % TEA_BLOCK_BYTES_ALGO != 0)
        return {};

    std::vector<uint8_t> padded(ciphertext.size());

    uint8_t prev[TEA_BLOCK_BYTES_ALGO];
    memcpy(prev, iv, TEA_BLOCK_BYTES_ALGO);

    for (size_t offset = 0; offset < ciphertext.size(); offset += TEA_BLOCK_BYTES_ALGO) {
        uint32_t v0, v1;
        unpack_block(&ciphertext[offset], &v0, &v1);
        tea_decrypt_block(tk, &v0, &v1);

        uint8_t block[TEA_BLOCK_BYTES_ALGO];
        pack_block(v0, v1, block);

        for (int b = 0; b < TEA_BLOCK_BYTES_ALGO; ++b)
            padded[offset + b] = block[b] ^ prev[b];

        memcpy(prev, &ciphertext[offset], TEA_BLOCK_BYTES_ALGO);
    }

    return pkcs7_unpad(padded);
}