#pragma once

#include <cstdint>
#include <vector>
#include <string>

using namespace std;

// =============================================================================
//  TEA — Tiny Encryption Algorithm (Уилер и Нидхэм, 1994)
//  Размер блока : 64 бит (два uint32_t — v0 и v1)
//  Длина ключа  : 128 бит (4 × uint32_t, фиксировано)
//  Раундов      : 64 (32 цикла × 2 операции)
//  Режим        : CBC + PKCS#7 паддинг
// =============================================================================

class TEA {
public:
    // ── Константы ─────────────────────────────────────────────────────────────
    static constexpr uint32_t DELTA      = 0x9e3779b9; // дробная часть золотого сечения
    static constexpr int      ROUNDS     = 32;          // циклов (каждый = 2 раунда Фейстеля)
    static constexpr int      KEY_WORDS  = 4;           // ключ: 4 × uint32_t
    static constexpr int      KEY_BYTES  = KEY_WORDS * sizeof(uint32_t); // 16 байт = 128 бит
    static constexpr int      BLOCK_BYTES = 8;          // 64 бит = 8 байт

    // ── Жизненный цикл ────────────────────────────────────────────────────────

    TEA();

    // Загрузить ключ (ровно 16 байт).
    // Возвращает false если длина ключа не равна KEY_BYTES.
    bool setKey(const vector<uint8_t>& key);

    // ── Блочные операции ──────────────────────────────────────────────────────

    // Зашифровать один блок: изменяет v0 и v1 на месте.
    void encryptBlock(uint32_t& v0, uint32_t& v1) const;

    // Расшифровать один блок: изменяет v0 и v1 на месте.
    void decryptBlock(uint32_t& v0, uint32_t& v1) const;

    // ── CBC-режим (произвольная длина данных) ─────────────────────────────────

    // Зашифровать данные в режиме CBC + PKCS#7 паддинг.
    // iv — вектор инициализации, ровно BLOCK_BYTES байт.
    vector<uint8_t> encryptCBC(const vector<uint8_t>& plaintext,
                                const vector<uint8_t>& iv) const;

    // Расшифровать данные в режиме CBC, снять PKCS#7 паддинг.
    // Бросает runtime_error при повреждённых данных или неверном паддинге.
    vector<uint8_t> decryptCBC(const vector<uint8_t>& ciphertext,
                                const vector<uint8_t>& iv) const;

private:
    // ── Внутреннее состояние ──────────────────────────────────────────────────

    uint32_t m_key[KEY_WORDS] = {0, 0, 0, 0};
    bool     m_keyIsSet       = false;

    // ── Вспомогательные методы ────────────────────────────────────────────────

    // PKCS#7: дополнить до кратности BLOCK_BYTES.
    static vector<uint8_t> pkcs7Pad(const vector<uint8_t>& data);

    // PKCS#7: снять паддинг. Бросает runtime_error при нарушении формата.
    static vector<uint8_t> pkcs7Unpad(const vector<uint8_t>& data);

    // Упаковать два uint32_t в 8 байт (big-endian).
    static void packBlock(uint32_t v0, uint32_t v1, uint8_t* out);

    // Распаковать 8 байт в два uint32_t (big-endian).
    static void unpackBlock(const uint8_t* in, uint32_t& v0, uint32_t& v1);
};

// =============================================================================
//  Точка входа из main.cpp
// =============================================================================
void runTEA();