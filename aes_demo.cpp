// ключ            2b7e151628aed2a6abf7158809cf4f3c
// открытый текст  3243f6a8885a308d313198a2e0370734

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std;

// ТАБЛИЦЫ

const unsigned char s_box[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

const unsigned char inv_s_box[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

const unsigned char Rcon[11] = {
    0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36
};

// КОНСТАНТЫ

const int Nb = 4;
const int Nk = 4;
const int Nr = 10;

const int BLOCK_SIZE = 16;
const int ROUND_KEYS_SIZE = Nb * 4 * (Nr + 1); // 176

// ЛОГГЕР

ofstream g_logFile;

void log_line(const string& text) {
    g_logFile << text << "\n";
}

void log_blank() {
    g_logFile << "\n";
}

string to_hex_byte(unsigned char b) {
    stringstream ss;
    ss << hex << setw(2) << setfill('0') << (int)b;
    return ss.str();
}

string bytes_to_hex(const unsigned char* data, int len) {
    stringstream ss;
    for (int i = 0; i < len; i++) {
        ss << to_hex_byte(data[i]);
        if (i != len - 1) ss << " ";
    }
    return ss.str();
}

// Печатает state[4][4]:
void print_matrix(const string& label, const unsigned char state[4][4]) {
    log_line(label + ":");
    for (int r = 0; r < 4; r++) {
        string row = "    ";
        for (int c = 0; c < 4; c++) {
            row += to_hex_byte(state[r][c]) + " ";
        }
        log_line(row);
    }
}

// bytes_to_matrix / matrix_to_bytes
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

// Внутренние операции раунда
void add_round_key(unsigned char state[4][4], const unsigned char* round_keys, int round) {
    log_line("  >> AddRoundKey (раунд " + to_string(round) + "): state = state XOR раундовый_ключ.");
    log_line("     Единственное место, где в шифрование подмешивается секретный ключ.");

    unsigned char keyMatrix[4][4];
    bytes_to_matrix(round_keys + round * BLOCK_SIZE, keyMatrix);
    print_matrix("     Раундовый ключ #" + to_string(round), keyMatrix);

    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < Nb; ++c)
            state[r][c] ^= round_keys[round * Nb * 4 + c * 4 + r];

    print_matrix("     Состояние после AddRoundKey", state);
}

void sub_bytes(unsigned char state[4][4]) {
    log_line("  >> SubBytes: каждый байт state заменяется значением из s_box.");
    log_line("     Единственная нелинейная операция AES -- даёт 'confusion'.");
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < Nb; ++c)
            state[r][c] = s_box[state[r][c]];
    print_matrix("     Состояние после SubBytes", state);
}

void inv_sub_bytes(unsigned char state[4][4]) {
    log_line("  >> InvSubBytes: обратная замена байт по inv_s_box (отменяет SubBytes).");
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < Nb; ++c)
            state[r][c] = inv_s_box[state[r][c]];
    print_matrix("     Состояние после InvSubBytes", state);
}

void shift_rows(unsigned char state[4][4]) {
    log_line("  >> ShiftRows: строка r сдвигается циклически влево на r позиций.");
    log_line("     Рассеивание (diffusion) -- байты разных столбцов перемешиваются.");
    unsigned char tmp[4];
    for (int r = 1; r < 4; ++r) {
        for (int c = 0; c < Nb; ++c) tmp[c] = state[r][(c + r) % Nb];
        for (int c = 0; c < Nb; ++c) state[r][c] = tmp[c];
    }
    print_matrix("     Состояние после ShiftRows", state);
}

void inv_shift_rows(unsigned char state[4][4]) {
    log_line("  >> InvShiftRows: тот же сдвиг, но вправо (отменяет ShiftRows).");
    unsigned char tmp[4];
    for (int r = 1; r < 4; ++r) {
        for (int c = 0; c < Nb; ++c) tmp[c] = state[r][(c + Nb - r) % Nb];
        for (int c = 0; c < Nb; ++c) state[r][c] = tmp[c];
    }
    print_matrix("     Состояние после InvShiftRows", state);
}

unsigned char gf_mul(unsigned char a, unsigned char b) {
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

void mix_columns(unsigned char state[4][4]) {
    log_line("  >> MixColumns: каждый столбец умножается на фиксированную матрицу в GF(2^8).");
    log_line("     Завершает рассеивание -- один байт начинает влиять на весь столбец.");
    unsigned char tmp[4];
    for (int c = 0; c < Nb; ++c) {
        tmp[0] = gf_mul(state[0][c], 2) ^ gf_mul(state[1][c], 3) ^ state[2][c]            ^ state[3][c];
        tmp[1] = state[0][c]            ^ gf_mul(state[1][c], 2) ^ gf_mul(state[2][c], 3) ^ state[3][c];
        tmp[2] = state[0][c]            ^ state[1][c]            ^ gf_mul(state[2][c], 2) ^ gf_mul(state[3][c], 3);
        tmp[3] = gf_mul(state[0][c], 3) ^ state[1][c]            ^ state[2][c]            ^ gf_mul(state[3][c], 2);
        for (int r = 0; r < 4; ++r) state[r][c] = tmp[r];
    }
    print_matrix("     Состояние после MixColumns", state);
}

void inv_mix_columns(unsigned char state[4][4]) {
    log_line("  >> InvMixColumns: умножение на обратную матрицу в GF(2^8) (отменяет MixColumns).");
    unsigned char tmp[4];
    for (int c = 0; c < Nb; ++c) {
        tmp[0] = gf_mul(state[0][c], 14) ^ gf_mul(state[1][c], 11) ^ gf_mul(state[2][c], 13) ^ gf_mul(state[3][c], 9);
        tmp[1] = gf_mul(state[0][c], 9)  ^ gf_mul(state[1][c], 14) ^ gf_mul(state[2][c], 11) ^ gf_mul(state[3][c], 13);
        tmp[2] = gf_mul(state[0][c], 13) ^ gf_mul(state[1][c], 9)  ^ gf_mul(state[2][c], 14) ^ gf_mul(state[3][c], 11);
        tmp[3] = gf_mul(state[0][c], 11) ^ gf_mul(state[1][c], 13) ^ gf_mul(state[2][c], 9)  ^ gf_mul(state[3][c], 14);
        for (int r = 0; r < 4; ++r) state[r][c] = tmp[r];
    }
    print_matrix("     Состояние после InvMixColumns", state);
}

// expand_key

void expand_key(const unsigned char* cipher_key, unsigned char* round_keys) {
    log_line("=========================================================");
    log_line("ШАГ 0: РАЗВОРАЧИВАНИЕ КЛЮЧА (expand_key)");
    log_line("=========================================================");

    // Первый раундовый ключ = мастер-ключ
    for (int i = 0; i < Nk * 4; ++i)
        round_keys[i] = cipher_key[i];

    log_line("Раундовый ключ #0 (= исходный ключ): " + bytes_to_hex(round_keys, 16));
    log_blank();

    unsigned char tmp[4];
    for (int i = Nk; i < Nb * (Nr + 1); ++i) {

        for (int j = 0; j < 4; ++j)
            tmp[j] = round_keys[(i - 1) * 4 + j];

        if (i % Nk == 0) {
            log_line("  Слово #" + to_string(i) + " -- граница раундового ключа, "
                      "применяем RotWord + SubWord + Rcon:");
            log_line("    до преобразования: " + bytes_to_hex(tmp, 4));

            // RotWord: циклический сдвиг влево
            unsigned char k = tmp[0];
            tmp[0] = tmp[1]; tmp[1] = tmp[2]; tmp[2] = tmp[3]; tmp[3] = k;
            log_line("    после RotWord:      " + bytes_to_hex(tmp, 4));

            // SubWord: замена через S-Box
            for (int j = 0; j < 4; ++j) tmp[j] = s_box[tmp[j]];
            log_line("    после SubWord:      " + bytes_to_hex(tmp, 4));

            // XOR с константой раунда
            tmp[0] ^= Rcon[i / Nk];
            log_line("    XOR с Rcon[" + to_string(i / Nk) + "]=" + to_hex_byte(Rcon[i / Nk])
                      + ":   " + bytes_to_hex(tmp, 4));
        }

        for (int j = 0; j < 4; ++j)
            round_keys[i * 4 + j] = round_keys[(i - Nk) * 4 + j] ^ tmp[j];

        if ((i + 1) % Nk == 0) {
            int roundNum = (i + 1) / Nk - 1;
            log_line("  Раундовый ключ #" + to_string(roundNum) + ": "
                      + bytes_to_hex(round_keys + roundNum * 16, 16));
            log_blank();
        }
    }

    log_line("Итого получено " + to_string(Nr + 1) + " раундовых ключей по 16 байт "
              "(" + to_string(ROUND_KEYS_SIZE) + " байт всего).");
    log_blank();
}

// encrypt_block

void encrypt_block(const unsigned char* input, unsigned char* output, const unsigned char* round_keys) {
    unsigned char state[4][4];
    bytes_to_matrix(input, state);

    log_line("=========================================================");
    log_line("ШИФРОВАНИЕ БЛОКА (encrypt_block)");
    log_line("=========================================================");
    print_matrix("Исходный открытый текст (state)", state);
    log_blank();

    log_line("--- Начальный AddRoundKey (round = 0) ---");
    add_round_key(state, round_keys, 0);
    log_blank();

    for (int round = 1; round < Nr; ++round) {
        log_line("--- РАУНД " + to_string(round) + " из " + to_string(Nr - 1)
                  + " (с MixColumns) ---");
        sub_bytes(state);
        shift_rows(state);
        mix_columns(state);
        add_round_key(state, round_keys, round);
        log_blank();
    }

    log_line("--- ПОСЛЕДНИЙ РАУНД " + to_string(Nr) + " (без MixColumns) ---");
    sub_bytes(state);
    shift_rows(state);
    add_round_key(state, round_keys, Nr);
    log_blank();

    matrix_to_bytes(state, output);

    log_line("=========================================================");
    print_matrix("РЕЗУЛЬТАТ -- зашифрованный блок", state);
    log_line("=========================================================");
    log_blank();
}

// decrypt_block

void decrypt_block(const unsigned char* input, unsigned char* output, const unsigned char* round_keys) {
    unsigned char state[4][4];
    bytes_to_matrix(input, state);

    log_line("=========================================================");
    log_line("РАСШИФРОВКА БЛОКА (decrypt_block)");
    log_line("=========================================================");
    print_matrix("Шифротекст на входе (state)", state);
    log_blank();

    log_line("--- Начальный AddRoundKey (round = " + to_string(Nr) + ") ---");
    add_round_key(state, round_keys, Nr);
    log_blank();

    for (int round = Nr - 1; round > 0; --round) {
        log_line("--- ОБРАТНЫЙ РАУНД (соответствует прямому раунду " + to_string(round) + ") ---");
        inv_shift_rows(state);
        inv_sub_bytes(state);
        add_round_key(state, round_keys, round);
        inv_mix_columns(state);
        log_blank();
    }

    log_line("--- ПОСЛЕДНИЙ ОБРАТНЫЙ РАУНД (без InvMixColumns) ---");
    inv_shift_rows(state);
    inv_sub_bytes(state);
    add_round_key(state, round_keys, 0);
    log_blank();

    matrix_to_bytes(state, output);

    log_line("=========================================================");
    print_matrix("РЕЗУЛЬТАТ -- расшифрованный блок", state);
    log_line("=========================================================");
    log_blank();
}

// Ввод hex-строки

bool parse_hex_16(const string& text, unsigned char out[BLOCK_SIZE]) {
    string clean;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] != ' ') clean += text[i];
    }
    if (clean.size() != 32) return false;

    for (int i = 0; i < BLOCK_SIZE; i++) {
        string byteStr = clean.substr(i * 2, 2);
        out[i] = (unsigned char)strtol(byteStr.c_str(), nullptr, 16);
    }
    return true;
}


int main() {
    unsigned char key[BLOCK_SIZE] = {
        0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
        0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c
    };
    unsigned char plaintext[BLOCK_SIZE] = {
        0x32,0x43,0xf6,0xa8,0x88,0x5a,0x30,0x8d,
        0x31,0x31,0x98,0xa2,0xe0,0x37,0x07,0x34
    };

    cout << "=========================================================\n";
    cout << " Пошаговая демонстрация AES-128\n";
    cout << "=========================================================\n";
    cout << "По умолчанию используется тестовый вектор FIPS-197.\n";
    cout << "Хотите ввести свой ключ и текст? (1 - да, 0 - нет, по умолчанию)\n> ";

    int choice = 0;
    cin >> choice;
    cin.ignore(10000, '\n');

    if (choice == 1) {
        string line;
        cout << "Введите ключ (32 hex-символа): ";
        getline(cin, line);
        unsigned char tmpKey[BLOCK_SIZE];
        if (parse_hex_16(line, tmpKey)) {
            for (int i = 0; i < BLOCK_SIZE; i++) key[i] = tmpKey[i];
        } else {
            cout << "Не удалось разобрать ключ, использую вектор по умолчанию.\n";
        }

        cout << "Введите открытый текст (32 hex-символа): ";
        getline(cin, line);
        unsigned char tmpPlain[BLOCK_SIZE];
        if (parse_hex_16(line, tmpPlain)) {
            for (int i = 0; i < BLOCK_SIZE; i++) plaintext[i] = tmpPlain[i];
        } else {
            cout << "Не удалось разобрать текст, использую вектор по умолчанию.\n";
        }
    }

    g_logFile.open("aes_steps.txt");
    if (!g_logFile.is_open()) {
        cout << "Не удалось создать файл aes_steps.txt\n";
        return 1;
    }

    log_line("=========================================================");
    log_line(" ПОДРОБНЫЙ ЛОГ РАБОТЫ AES-128 (один блок, 16 байт)");
    log_line("=========================================================");
    log_line("Ключ:           " + bytes_to_hex(key, BLOCK_SIZE));
    log_line("Открытый текст: " + bytes_to_hex(plaintext, BLOCK_SIZE));
    log_blank();

    unsigned char round_keys[ROUND_KEYS_SIZE];
    expand_key(key, round_keys);

    unsigned char ciphertext[BLOCK_SIZE];
    encrypt_block(plaintext, ciphertext, round_keys);

    unsigned char decrypted[BLOCK_SIZE];
    decrypt_block(ciphertext, decrypted, round_keys);

    log_line("=========================================================");
    log_line("ИТОГ");
    log_line("=========================================================");
    log_line("Открытый текст (исходный):   " + bytes_to_hex(plaintext, BLOCK_SIZE));
    log_line("Шифротекст:                  " + bytes_to_hex(ciphertext, BLOCK_SIZE));
    log_line("Расшифровано обратно:        " + bytes_to_hex(decrypted, BLOCK_SIZE));

    bool ok = true;
    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (plaintext[i] != decrypted[i]) ok = false;
    }
    log_line(string("Совпадает с исходным текстом: ") + (ok ? "ДА (всё верно)" : "НЕТ (ошибка!)"));

    g_logFile.close();

    cout << "\nГотово! Полный лог сохранён в файл aes_steps.txt\n";

    return 0;
}