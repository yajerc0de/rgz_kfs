#include "serpent_cbc.h"
#include "serpent_core.h"

#include <cstring>

std::vector<unsigned char> serpent_cbc_encrypt(
    const std::vector<unsigned char>& plaintext,
    const unsigned char* iv,
    const unsigned int* subkeys)
{
    // Добавляем PKCS#7 паддинг, чтобы длина данных была кратна BLOCK_SIZE
    std::vector<unsigned char> data = plaintext;
    int pad_len = BLOCK_SIZE - (int)(data.size() % BLOCK_SIZE);
    for (int i = 0; i < pad_len; ++i)
        data.push_back((unsigned char)pad_len);

    std::vector<unsigned char> ciphertext(data.size());

    unsigned char prev[BLOCK_SIZE];
    memcpy(prev, iv, BLOCK_SIZE);

    for (int offset = 0; offset < (int)data.size(); offset += BLOCK_SIZE) {
        unsigned char block[BLOCK_SIZE];

        for (int i = 0; i < BLOCK_SIZE; ++i)
            block[i] = data[offset + i] ^ prev[i];

        serpent_encrypt_block(block, &ciphertext[offset], subkeys);

        memcpy(prev, &ciphertext[offset], BLOCK_SIZE);
    }

    return ciphertext;
}

std::vector<unsigned char> serpent_cbc_decrypt(
    const std::vector<unsigned char>& ciphertext,
    const unsigned char* iv,
    const unsigned int* subkeys)
{
    std::vector<unsigned char> result(ciphertext.size());

    unsigned char prev[BLOCK_SIZE];
    memcpy(prev, iv, BLOCK_SIZE);

    for (int offset = 0; offset < (int)ciphertext.size(); offset += BLOCK_SIZE) {
        unsigned char decrypted[BLOCK_SIZE];
        serpent_decrypt_block(&ciphertext[offset], decrypted, subkeys);

        for (int i = 0; i < BLOCK_SIZE; ++i)
            result[offset + i] = decrypted[i] ^ prev[i];

        memcpy(prev, &ciphertext[offset], BLOCK_SIZE);
    }

    if (!result.empty()) {
        int pad = result.back();
        if (pad > 0 && pad <= BLOCK_SIZE)
            result.resize(result.size() - pad);
    }

    return result;
}
