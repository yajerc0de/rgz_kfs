#pragma once

#include <cstdint>
#include <string>
#include <vector>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
//  Blowfish — симметричный блочный шифр (Брюс Шнайер, 1993)
//  Размер блока : 64 бит (два uint32_t — L и R)
//  Длина ключа  : 32–448 бит (4–56 байт)
//  Раундов      : 16
//  Режим        : CBC + PKCS#7 паддинг
// ─────────────────────────────────────────────────────────────────────────────

class Blowfish {
public:
    // ── Константы ─────────────────────────────────────────────────────────────
    static constexpr int ROUNDS       = 16;
    static constexpr int P_ARRAY_SIZE = ROUNDS + 2;   // P[0..17]
    static constexpr int S_BOX_COUNT  = 4;
    static constexpr int S_BOX_SIZE   = 256;
    static constexpr int BLOCK_BYTES  = 8;            // 64 бит = 8 байт
    static constexpr int KEY_MIN      = 4;            // байт
    static constexpr int KEY_MAX      = 56;           // байт

    // ── Жизненный цикл ────────────────────────────────────────────────────────

    Blowfish();

    // Загрузить ключ и выполнить Key Schedule.
    // key — сырые байты, длина KEY_MIN..KEY_MAX.
    // Возвращает false если длина ключа вне допустимого диапазона.
    bool setKey(const vector<uint8_t>& key);

    // ── Блочные операции (открытый интерфейс для тестирования) ────────────────

    // Зашифровать один блок: изменяет L и R на месте.
    void encryptBlock(uint32_t& L, uint32_t& R) const;

    // Расшифровать один блок: изменяет L и R на месте.
    void decryptBlock(uint32_t& L, uint32_t& R) const;

    // ── CBC-режим (произвольная длина данных) ─────────────────────────────────

    // Зашифровать данные в режиме CBC.
    // iv — вектор инициализации, ровно BLOCK_BYTES байт.
    // Применяет PKCS#7 паддинг, возвращает шифротекст.
    vector<uint8_t> encryptCBC(const vector<uint8_t>& plaintext,
                                const vector<uint8_t>& iv) const;

    // Расшифровать данные в режиме CBC.
    // Снимает PKCS#7 паддинг, возвращает исходный открытый текст.
    // Бросает runtime_error при повреждённых данных или неверном паддинге.
    vector<uint8_t> decryptCBC(const vector<uint8_t>& ciphertext,
                                const vector<uint8_t>& iv) const;

private:
    // ── Внутреннее состояние ──────────────────────────────────────────────────

    uint32_t P[P_ARRAY_SIZE];
    uint32_t S[S_BOX_COUNT][S_BOX_SIZE];

    bool     m_keyIsSet = false;

    // ── Вспомогательные методы ────────────────────────────────────────────────

    // Функция F — нелинейное преобразование 32-битного слова через S-блоки.
    uint32_t F(uint32_t x) const;

    // PKCS#7: дополнить данные до кратности BLOCK_BYTES.
    static vector<uint8_t> pkcs7Pad(const vector<uint8_t>& data);

    // PKCS#7: убрать паддинг. Бросает runtime_error при нарушении формата.
    static vector<uint8_t> pkcs7Unpad(const vector<uint8_t>& data);

    // Упаковать/распаковать два uint32_t в/из 8 байт (big-endian).
    static void   packBlock  (uint32_t L, uint32_t R, uint8_t* out);
    static void   unpackBlock(const uint8_t* in, uint32_t& L, uint32_t& R);

    // ── Константы инициализации (дробные части π) ─────────────────────────────
    // Объявлены как static — хранятся один раз для всей программы.

    static const uint32_t INIT_P[P_ARRAY_SIZE];
    static const uint32_t INIT_S[S_BOX_COUNT][S_BOX_SIZE];
};

// ─────────────────────────────────────────────────────────────────────────────
//  Точка входа из main.cpp
//  Реализована в blowfish.cpp — показывает меню: текст / файл / генератор ключа
// ─────────────────────────────────────────────────────────────────────────────
void runBlowfish();