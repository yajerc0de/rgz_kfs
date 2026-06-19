#pragma once

#include <cstdint>
#include <vector>

// =============================================================================
//  Blowfish — симметричный блочный шифр (Брюс Шнайер, 1993)
//  Размер блока : 64 бит (два uint32_t — L и R)
//  Длина ключа  : 32–448 бит (4–56 байт)
//  Раундов      : 16
//  Режим        : CBC + PKCS#7 паддинг
// =============================================================================

// ── Константы ─────────────────────────────────────────────────────────────────
static constexpr int BF_ROUNDS        = 16;
static constexpr int BF_P_ARRAY_SIZE  = BF_ROUNDS + 2;  // P[0..17]
static constexpr int BF_S_BOX_COUNT   = 4;
static constexpr int BF_S_BOX_SIZE    = 256;
static constexpr int BF_BLOCK_BYTES   = 8;               // 64 бит = 8 байт
static constexpr int BF_KEY_MIN       = 4;               // байт
static constexpr int BF_KEY_MAX       = 56;              // байт

// ── Внутреннее состояние (ключ + S-блоки после Key Schedule) ─────────────────
// Используется только в algo/ — capi/ не включает этот заголовок напрямую.
struct BFKey {
    uint32_t P[BF_P_ARRAY_SIZE];
    uint32_t S[BF_S_BOX_COUNT][BF_S_BOX_SIZE];
    bool     ready;
};

// ── Таблицы инициализации (дробные части π) ───────────────────────────────────
// Объявлены extern — определены в blowfish.cpp одним экземпляром.
extern const uint32_t BF_INIT_P[BF_P_ARRAY_SIZE];
extern const uint32_t BF_INIT_S[BF_S_BOX_COUNT][BF_S_BOX_SIZE];

// ── Инициализация ─────────────────────────────────────────────────────────────

// Сбросить структуру BFKey в начальное состояние (таблицы π, ready = false).
void bf_key_init(BFKey* bk);

// Загрузить ключ и выполнить Key Schedule.
// key — сырые байты, длина BF_KEY_MIN..BF_KEY_MAX.
// Возвращает false если key == nullptr или длина вне допустимого диапазона.
bool bf_key_set(BFKey* bk, const uint8_t* key, int keyLen);

// ── Блочные операции ──────────────────────────────────────────────────────────

// Зашифровать один 64-битный блок. Изменяет L и R на месте.
void bf_encrypt_block(const BFKey* bk, uint32_t* L, uint32_t* R);

// Расшифровать один 64-битный блок. Изменяет L и R на месте.
void bf_decrypt_block(const BFKey* bk, uint32_t* L, uint32_t* R);

// ── CBC-режим (произвольная длина данных) ─────────────────────────────────────

// Зашифровать данные в режиме CBC + PKCS#7 паддинг.
// iv — вектор инициализации, ровно BF_BLOCK_BYTES байт.
// Возвращает зашифрованный вектор или пустой вектор при ошибке.
std::vector<uint8_t> bf_cbc_encrypt(
    const BFKey*               bk,
    const std::vector<uint8_t>& plaintext,
    const uint8_t*              iv);

// Расшифровать данные в режиме CBC, снять PKCS#7 паддинг.
// Возвращает открытый текст или пустой вектор при ошибке
// (неверный паддинг, неверный ключ, повреждённые данные).
std::vector<uint8_t> bf_cbc_decrypt(
    const BFKey*               bk,
    const std::vector<uint8_t>& ciphertext,
    const uint8_t*              iv);