#pragma once

#include "../capi/aes_capi.h"
#include <cstring>

// Константы AES-128
const int Nb = 4;
const int Nk = 4;
const int Nr = 10;

const int BLOCK_SIZE = 16;
const int ROUND_KEYS_SIZE = Nb * 4 * (Nr + 1);

// Массив байт -> State-матрица (4x4)
AES_API void bytes_to_matrix(const unsigned char* input, unsigned char state[4][4]);

// State-матрица -> массив байт
AES_API void matrix_to_bytes(const unsigned char state[4][4], unsigned char* output);

// Разворачивание ключа (Key Expansion)
AES_API void expand_key(const unsigned char* cipher_key, unsigned char* round_keys);

// Шифрование одного блока (16 байт)
AES_API void encrypt_block(const unsigned char* input, unsigned char* output, const unsigned char* round_keys);

// Дешифрование одного блока (16 байт)
AES_API void decrypt_block(const unsigned char* input, unsigned char* output, const unsigned char* round_keys);