#pragma once

// Размер блока Serpent
const int BLOCK_SIZE = 16;

// Размер ключа
const int KEY_SIZE = 16;

// Количество раундов в Serpent
const int NUM_ROUNDS = 32;

// Количество 32-битных подключей
const int SUBKEYS_COUNT = 132;

// Разворачивание 128-битного ключа в 132 подключа по 32 бита
// cipher_key  - входной ключ, 16 байт
// subkeys     - выходной массив на 132 элемента (unsigned int)
void serpent_expand_key(const unsigned char* cipher_key, unsigned int* subkeys);

// Шифрование одного блока (16 байт)
void serpent_encrypt_block(const unsigned char* input, unsigned char* output, const unsigned int* subkeys);

// Дешифрование одного блока (16 байт)
void serpent_decrypt_block(const unsigned char* input, unsigned char* output, const unsigned int* subkeys);
