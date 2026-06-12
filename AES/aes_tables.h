#pragma once

// Нелинейная таблица замен (S-Box)
extern const unsigned char s_box[256];

// Инвертированный S-Box
extern const unsigned char inv_s_box[256];

// Константы раундов (Rcon)
extern const unsigned char Rcon[11];
