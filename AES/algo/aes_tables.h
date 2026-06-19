#pragma once

#include "../capi/aes_capi.h"

// Нелинейная таблица замен (S-Box)
extern AES_API const unsigned char s_box[256];

// Инвертированный S-Box
extern AES_API const unsigned char inv_s_box[256];

// Константы раундов (Rcon)
extern AES_API const unsigned char Rcon[11];