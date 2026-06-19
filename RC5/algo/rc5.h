#pragma once

#include <cstdint>
#include <vector>
#include <string>

using namespace std;

// =============================================================================
//  RC5 — Rivest Cipher 5 (Ривест, 1994)
//  Вариант       : RC5-32/12/16 (слово 32 бит, 12 раундов, ключ до 255 байт)
//  Размер блока  : 64 бит (два uint32_t — A и B)
//  Длина ключа   : 1–255 байт (по умолчанию 16 байт = 128 бит)
//  Раундов       : 12
//  Режим         : CBC + PKCS#7 паддинг
//
//  Реализация без классов: расширенная таблица ключей хранится в обычной
//  структуре (POD), вся работа идёт через свободные функции.
// =============================================================================

// ── Константы ─────────────────────────────────────────────────────────────────

constexpr int      RC5_WORD_BITS   = 32;
constexpr uint32_t RC5_W           = RC5_WORD_BITS;
constexpr int      RC5_ROUNDS      = 12;
constexpr int      RC5_KEY_BYTES_DEFAULT = 16;
constexpr int      RC5_BLOCK_LEN   = 8;                       // 8 байт = 64 бит
constexpr int      RC5_TABLE_SIZE  = 2 * (RC5_ROUNDS + 1);    // 26 слов

// Магические константы P и Q для RC5-32 (Odd((e-2)*2^32), Odd((φ-1)*2^32))
constexpr uint32_t RC5_P32 = 0xB7E15163u;
constexpr uint32_t RC5_Q32 = 0x9E3779B9u;

// ── Состояние ключа ────────────────────────────────────────────────────────────
// Обычная структура данных (POD), без методов и конструкторов с поведением —
// заполняется и читается только свободными функциями ниже.

struct Rc5KeySchedule {
    uint32_t S[RC5_TABLE_SIZE];
    bool     keyIsSet;
};

// ── Жизненный цикл ────────────────────────────────────────────────────────────

// Обнулить структуру ключевого расписания.
void rc5InitSchedule(Rc5KeySchedule& sched);

// Развернуть ключ (1–255 байт) в таблицу sched.S.
// Возвращает false если длина ключа вне допустимого диапазона.
bool rc5SetKey(Rc5KeySchedule& sched, const vector<uint8_t>& key);

// ── Блочные операции ──────────────────────────────────────────────────────────

// Зашифровать один блок 64 бит: изменяет A и B на месте.
void rc5EncryptBlock(const Rc5KeySchedule& sched, uint32_t& A, uint32_t& B);

// Расшифровать один блок 64 бит: изменяет A и B на месте.
void rc5DecryptBlock(const Rc5KeySchedule& sched, uint32_t& A, uint32_t& B);

// ── CBC-режим (произвольная длина данных) ─────────────────────────────────────

// Зашифровать данные в режиме CBC + PKCS#7 паддинг.
// iv — вектор инициализации, ровно RC5_BLOCK_LEN байт.
vector<uint8_t> rc5EncryptCBC(const Rc5KeySchedule& sched,
                               const vector<uint8_t>& plaintext,
                               const vector<uint8_t>& iv);

// Расшифровать данные в режиме CBC, снять PKCS#7 паддинг.
// Бросает runtime_error при повреждённых данных или неверном паддинге.
vector<uint8_t> rc5DecryptCBC(const Rc5KeySchedule& sched,
                               const vector<uint8_t>& ciphertext,
                               const vector<uint8_t>& iv);

// =============================================================================
//  Точка входа из main.cpp
// =============================================================================
void runRC5();
