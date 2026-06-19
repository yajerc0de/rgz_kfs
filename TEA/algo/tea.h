#pragma once

#include <cstdint>
#include <vector>

// =============================================================================
//  TEA — Tiny Encryption Algorithm (Уилер и Нидхэм, 1994)
//  Размер блока : 64 бит (два uint32_t — v0 и v1)
//  Длина ключа  : 128 бит (4 × uint32_t, фиксировано)
//  Раундов      : 64 (32 цикла × 2 операции)
//  Режим        : CBC + PKCS#7 паддинг
// =============================================================================

// ── Константы ─────────────────────────────────────────────────────────────────
static constexpr uint32_t TEA_DELTA       = 0x9e3779b9u; // дробная часть золотого сечения
static constexpr int      TEA_ROUNDS      = 32;           // циклов (каждый = 2 раунда Фейстеля)
static constexpr int      TEA_KEY_WORDS   = 4;            // ключ: 4 × uint32_t
static constexpr int      TEA_KEY_BYTES   = TEA_KEY_WORDS * 4; // 16 байт = 128 бит
static constexpr int      TEA_BLOCK_BYTES_ALGO = 8;       // 64 бит = 8 байт

// ── Внутреннее состояние (ключ), заполняется через tea_algo_set_key() ─────────
// Используется только в algo/ — capi/ не включает этот заголовок напрямую.
struct TeaKey {
    uint32_t k[TEA_KEY_WORDS];
    bool     ready;
};

// ── Инициализация ─────────────────────────────────────────────────────────────

// Заполнить структуру TeaKey нулями (состояние "ключ не установлен").
void tea_key_init(TeaKey* tk);

// Загрузить ключ из массива байт (ровно TEA_KEY_BYTES = 16 байт, big-endian).
// Возвращает false если key == nullptr или keyLen != TEA_KEY_BYTES.
bool tea_key_set(TeaKey* tk, const uint8_t* key, int keyLen);

// ── Блочные операции ──────────────────────────────────────────────────────────

// Зашифровать один 64-битный блок. Изменяет v0 и v1 на месте.
void tea_encrypt_block(const TeaKey* tk, uint32_t* v0, uint32_t* v1);

// Расшифровать один 64-битный блок. Изменяет v0 и v1 на месте.
void tea_decrypt_block(const TeaKey* tk, uint32_t* v0, uint32_t* v1);

// ── CBC-режим (произвольная длина данных) ─────────────────────────────────────

// Зашифровать данные в режиме CBC + PKCS#7 паддинг.
// iv — вектор инициализации, ровно TEA_BLOCK_BYTES_ALGO байт.
// Возвращает зашифрованный вектор или пустой вектор при ошибке.
std::vector<uint8_t> tea_cbc_encrypt(
    const TeaKey*              tk,
    const std::vector<uint8_t>& plaintext,
    const uint8_t*              iv);

// Расшифровать данные в режиме CBC, снять PKCS#7 паддинг.
// Возвращает открытый текст или пустой вектор при ошибке
// (неверный паддинг, неверный ключ, повреждённые данные).
std::vector<uint8_t> tea_cbc_decrypt(
    const TeaKey*              tk,
    const std::vector<uint8_t>& ciphertext,
    const uint8_t*              iv);