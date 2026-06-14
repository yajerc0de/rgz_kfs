#include "aes_core.h"
#include "aes_tables.h"

// Вспомогательные внутренние функции

static void add_round_key(unsigned char state[4][4], const unsigned char* round_keys, int round) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < Nb; ++c)
            state[r][c] ^= round_keys[round * Nb * 4 + c * 4 + r];
}

static void sub_bytes(unsigned char state[4][4]) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < Nb; ++c)
            state[r][c] = s_box[state[r][c]];
}

static void inv_sub_bytes(unsigned char state[4][4]) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < Nb; ++c)
            state[r][c] = inv_s_box[state[r][c]];
}

static void shift_rows(unsigned char state[4][4]) {
    unsigned char tmp[4];
    for (int r = 1; r < 4; ++r) {
        for (int c = 0; c < Nb; ++c) tmp[c] = state[r][(c + r) % Nb];
        for (int c = 0; c < Nb; ++c) state[r][c] = tmp[c];
    }
}

static void inv_shift_rows(unsigned char state[4][4]) {
    unsigned char tmp[4];
    for (int r = 1; r < 4; ++r) {
        for (int c = 0; c < Nb; ++c) tmp[c] = state[r][(c + Nb - r) % Nb];
        for (int c = 0; c < Nb; ++c) state[r][c] = tmp[c];
    }
}

// Умножение в поле Галуа GF(2^8)
static unsigned char gf_mul(unsigned char a, unsigned char b) {
    unsigned char p = 0;
    for (int i = 0; i < 8; ++i) {
        if (b & 1) p ^= a;
        bool hi = (a & 0x80);
        a <<= 1;
        if (hi) a ^= 0x1B;
        b >>= 1;
    }
    return p;
}

static void mix_columns(unsigned char state[4][4]) {
    unsigned char tmp[4];
    for (int c = 0; c < Nb; ++c) {
        tmp[0] = gf_mul(state[0][c], 2) ^ gf_mul(state[1][c], 3) ^ state[2][c]            ^ state[3][c];
        tmp[1] = state[0][c]            ^ gf_mul(state[1][c], 2) ^ gf_mul(state[2][c], 3) ^ state[3][c];
        tmp[2] = state[0][c]            ^ state[1][c]            ^ gf_mul(state[2][c], 2) ^ gf_mul(state[3][c], 3);
        tmp[3] = gf_mul(state[0][c], 3) ^ state[1][c]            ^ state[2][c]            ^ gf_mul(state[3][c], 2);
        for (int r = 0; r < 4; ++r) state[r][c] = tmp[r];
    }
}

static void inv_mix_columns(unsigned char state[4][4]) {
    unsigned char tmp[4];
    for (int c = 0; c < Nb; ++c) {
        tmp[0] = gf_mul(state[0][c], 14) ^ gf_mul(state[1][c], 11) ^ gf_mul(state[2][c], 13) ^ gf_mul(state[3][c], 9);
        tmp[1] = gf_mul(state[0][c], 9)  ^ gf_mul(state[1][c], 14) ^ gf_mul(state[2][c], 11) ^ gf_mul(state[3][c], 13);
        tmp[2] = gf_mul(state[0][c], 13) ^ gf_mul(state[1][c], 9)  ^ gf_mul(state[2][c], 14) ^ gf_mul(state[3][c], 11);
        tmp[3] = gf_mul(state[0][c], 11) ^ gf_mul(state[1][c], 13) ^ gf_mul(state[2][c], 9)  ^ gf_mul(state[3][c], 14);
        for (int r = 0; r < 4; ++r) state[r][c] = tmp[r];
    }
}

// Публичные функции

void bytes_to_matrix(const unsigned char* input, unsigned char state[4][4]) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < Nb; ++c)
            state[r][c] = input[c * 4 + r];
}

void matrix_to_bytes(const unsigned char state[4][4], unsigned char* output) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < Nb; ++c)
            output[c * 4 + r] = state[r][c];
}

void expand_key(const unsigned char* cipher_key, unsigned char* round_keys) {
    // Первый раундовый ключ = мастер-ключ
    for (int i = 0; i < Nk * 4; ++i)
        round_keys[i] = cipher_key[i];

    unsigned char tmp[4];
    for (int i = Nk; i < Nb * (Nr + 1); ++i) {
        for (int j = 0; j < 4; ++j)
            tmp[j] = round_keys[(i - 1) * 4 + j];

        if (i % Nk == 0) {
            // RotWord: циклический сдвиг влево
            unsigned char k = tmp[0];
            tmp[0] = tmp[1]; tmp[1] = tmp[2]; tmp[2] = tmp[3]; tmp[3] = k;
            // SubWord: замена через S-Box
            for (int j = 0; j < 4; ++j) tmp[j] = s_box[tmp[j]];
            // XOR с константой раунда
            tmp[0] ^= Rcon[i / Nk];
        }

        for (int j = 0; j < 4; ++j)
            round_keys[i * 4 + j] = round_keys[(i - Nk) * 4 + j] ^ tmp[j];
    }
}

void encrypt_block(const unsigned char* input, unsigned char* output, const unsigned char* round_keys) {
    unsigned char state[4][4];
    bytes_to_matrix(input, state);

    add_round_key(state, round_keys, 0);

    for (int round = 1; round < Nr; ++round) {
        sub_bytes(state);
        shift_rows(state);
        mix_columns(state);
        add_round_key(state, round_keys, round);
    }

    sub_bytes(state);
    shift_rows(state);
    add_round_key(state, round_keys, Nr);

    matrix_to_bytes(state, output);
}

void decrypt_block(const unsigned char* input, unsigned char* output, const unsigned char* round_keys) {
    unsigned char state[4][4];
    bytes_to_matrix(input, state);

    add_round_key(state, round_keys, Nr);

    for (int round = Nr - 1; round > 0; --round) {
        inv_shift_rows(state);
        inv_sub_bytes(state);
        add_round_key(state, round_keys, round);
        inv_mix_columns(state);
    }

    inv_shift_rows(state);
    inv_sub_bytes(state);
    add_round_key(state, round_keys, 0);

    matrix_to_bytes(state, output);
}
