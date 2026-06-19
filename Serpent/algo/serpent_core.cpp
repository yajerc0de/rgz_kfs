#include "serpent_core.h"
#include "serpent_tables.h"

#include <cstring>

// Циклический сдвиг 32-битного числа влево на n бит
static unsigned int rotate_left(unsigned int x, int n) {
    return (x << n) | (x >> (32 - n));
}

// Циклический сдвиг 32-битного числа вправо на n бит
static unsigned int rotate_right(unsigned int x, int n) {
    return (x >> n) | (x << (32 - n));
}

// Применяет один S-бокс к 4 словам (X0,X1,X2,X3), обрабатывая их побитово.
static void apply_sbox(unsigned int& x0, unsigned int& x1, unsigned int& x2, unsigned int& x3,
                        int box_index, bool inverse)
{
    unsigned int out0 = 0, out1 = 0, out2 = 0, out3 = 0;

    for (int bit = 0; bit < 32; ++bit) {
        unsigned char nibble = 0;
        nibble |= (unsigned char)(((x0 >> bit) & 1) << 0);
        nibble |= (unsigned char)(((x1 >> bit) & 1) << 1);
        nibble |= (unsigned char)(((x2 >> bit) & 1) << 2);
        nibble |= (unsigned char)(((x3 >> bit) & 1) << 3);

        unsigned char result;
        if (inverse)
            result = serpent_sbox_inv[box_index][nibble];
        else
            result = serpent_sbox[box_index][nibble];

        out0 |= (unsigned int)((result >> 0) & 1) << bit;
        out1 |= (unsigned int)((result >> 1) & 1) << bit;
        out2 |= (unsigned int)((result >> 2) & 1) << bit;
        out3 |= (unsigned int)((result >> 3) & 1) << bit;
    }

    x0 = out0;
    x1 = out1;
    x2 = out2;
    x3 = out3;
}

// Линейное преобразование после S-box в раундах 0..30.
static void linear_transform(unsigned int& x0, unsigned int& x1, unsigned int& x2, unsigned int& x3) {
    x0 = rotate_left(x0, 13);
    x2 = rotate_left(x2, 3);
    x1 = x1 ^ x0 ^ x2;
    x3 = x3 ^ x2 ^ (x0 << 3);
    x1 = rotate_left(x1, 1);
    x3 = rotate_left(x3, 7);
    x0 = x0 ^ x1 ^ x3;
    x2 = x2 ^ x3 ^ (x1 << 7);
    x0 = rotate_left(x0, 5);
    x2 = rotate_left(x2, 22);
}

// Обратное линейное преобразование
static void inverse_linear_transform(unsigned int& x0, unsigned int& x1, unsigned int& x2, unsigned int& x3) {
    x2 = rotate_right(x2, 22);
    x0 = rotate_right(x0, 5);
    x2 = x2 ^ x3 ^ (x1 << 7);
    x0 = x0 ^ x1 ^ x3;
    x3 = rotate_right(x3, 7);
    x1 = rotate_right(x1, 1);
    x3 = x3 ^ x2 ^ (x0 << 3);
    x1 = x1 ^ x0 ^ x2;
    x2 = rotate_right(x2, 3);
    x0 = rotate_right(x0, 13);
}

// XOR блока (x0,x1,x2,x3) с подключом
static void xor_with_subkey(unsigned int& x0, unsigned int& x1, unsigned int& x2, unsigned int& x3,
                             const unsigned int* subkeys, int key_index)
{
    x0 ^= subkeys[key_index * 4 + 0];
    x1 ^= subkeys[key_index * 4 + 1];
    x2 ^= subkeys[key_index * 4 + 2];
    x3 ^= subkeys[key_index * 4 + 3];
}

// Превращает 16 байт в 4 слова по 32 бита
static void bytes_to_words(const unsigned char* input, unsigned int& x0, unsigned int& x1,
                            unsigned int& x2, unsigned int& x3)
{
    x0 = (unsigned int)input[0]  | ((unsigned int)input[1]  << 8) | ((unsigned int)input[2]  << 16) | ((unsigned int)input[3]  << 24);
    x1 = (unsigned int)input[4]  | ((unsigned int)input[5]  << 8) | ((unsigned int)input[6]  << 16) | ((unsigned int)input[7]  << 24);
    x2 = (unsigned int)input[8]  | ((unsigned int)input[9]  << 8) | ((unsigned int)input[10] << 16) | ((unsigned int)input[11] << 24);
    x3 = (unsigned int)input[12] | ((unsigned int)input[13] << 8) | ((unsigned int)input[14] << 16) | ((unsigned int)input[15] << 24);
}

// Превращает 4 слова по 32 бита обратно в 16 байт
static void words_to_bytes(unsigned int x0, unsigned int x1, unsigned int x2, unsigned int x3,
                            unsigned char* output)
{
    for (int i = 0; i < 4; ++i) output[0 + i]  = (unsigned char)(x0 >> (8 * i));
    for (int i = 0; i < 4; ++i) output[4 + i]  = (unsigned char)(x1 >> (8 * i));
    for (int i = 0; i < 4; ++i) output[8 + i]  = (unsigned char)(x2 >> (8 * i));
    for (int i = 0; i < 4; ++i) output[12 + i] = (unsigned char)(x3 >> (8 * i));
}

// Разворачивание ключа (Key Schedule)

const unsigned int PHI = 0x9E3779B9u;

void serpent_expand_key(const unsigned char* cipher_key, unsigned int* subkeys) {
    unsigned int w[8 + 132];

    unsigned int k0, k1, k2, k3;
    bytes_to_words(cipher_key, k0, k1, k2, k3);

    w[-8 + 8] = k0;
    w[-7 + 8] = k1;
    w[-6 + 8] = k2;
    w[-5 + 8] = k3;
    w[-4 + 8] = 1;
    w[-3 + 8] = 0;
    w[-2 + 8] = 0;
    w[-1 + 8] = 0;

    for (int i = 0; i < 132; ++i) {
        unsigned int val = w[(i - 8) + 8] ^ w[(i - 5) + 8] ^ w[(i - 3) + 8] ^ w[(i - 1) + 8] ^ PHI ^ (unsigned int)i;
        w[i + 8] = rotate_left(val, 11);
    }

    for (int i = 0; i < 33; ++i) {
        unsigned int x0 = w[i * 4 + 0 + 8];
        unsigned int x1 = w[i * 4 + 1 + 8];
        unsigned int x2 = w[i * 4 + 2 + 8];
        unsigned int x3 = w[i * 4 + 3 + 8];

        int box = (35 - i) % 8;
        apply_sbox(x0, x1, x2, x3, box, false);

        subkeys[i * 4 + 0] = x0;
        subkeys[i * 4 + 1] = x1;
        subkeys[i * 4 + 2] = x2;
        subkeys[i * 4 + 3] = x3;
    }
}

// Шифрование / дешифрование блока

void serpent_encrypt_block(const unsigned char* input, unsigned char* output, const unsigned int* subkeys) {
    unsigned int x0, x1, x2, x3;
    bytes_to_words(input, x0, x1, x2, x3);

    for (int round = 0; round < NUM_ROUNDS; ++round) {
        xor_with_subkey(x0, x1, x2, x3, subkeys, round);
        apply_sbox(x0, x1, x2, x3, round % 8, false);
        if (round < NUM_ROUNDS - 1) {
            linear_transform(x0, x1, x2, x3);
        }
    }

    xor_with_subkey(x0, x1, x2, x3, subkeys, NUM_ROUNDS);
    words_to_bytes(x0, x1, x2, x3, output);
}

void serpent_decrypt_block(const unsigned char* input, unsigned char* output, const unsigned int* subkeys) {
    unsigned int x0, x1, x2, x3;
    bytes_to_words(input, x0, x1, x2, x3);

    xor_with_subkey(x0, x1, x2, x3, subkeys, NUM_ROUNDS);

    for (int round = NUM_ROUNDS - 1; round >= 0; --round) {
        if (round < NUM_ROUNDS - 1) {
            inverse_linear_transform(x0, x1, x2, x3);
        }
        apply_sbox(x0, x1, x2, x3, round % 8, true);
        xor_with_subkey(x0, x1, x2, x3, subkeys, round);
    }

    words_to_bytes(x0, x1, x2, x3, output);
}
