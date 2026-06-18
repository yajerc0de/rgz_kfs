#pragma once

#include <serpent_export.h>

// Serpent использует 8 разных S-боксов, каждый из 16 значений.
// Каждый S-бокс применяется 4 раза за 32 раунда (раунд i использует S-бокс (i % 8)).

// Прямые S-боксы (для шифрования)
extern SERPENT_API const unsigned char serpent_sbox[8][16];

// Обратные S-боксы (для дешифрования)
extern SERPENT_API const unsigned char serpent_sbox_inv[8][16];
