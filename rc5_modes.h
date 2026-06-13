#ifndef RC5_MODES_H
#define RC5_MODES_H

#include <stdint.h>
#include <stddef.h>
#include "rc5.h"

/*
 * RC5 в режиме CBC, блок = 8 байт.
 * Набивка последнего блока: PKCS#7 (1..8 байт).
 *
 * Шифрование: возвращает длину зашифрованных данных.
 * Дешифрование: возвращает длину открытых данных, или -1 при ошибке.
 */

int rc5_cbc_encrypt(const RC5_CTX *ctx,
                    const uint8_t *src, size_t src_len,
                    uint8_t *dst,
                    const uint8_t iv[8]);

int rc5_cbc_decrypt(const RC5_CTX *ctx,
                    const uint8_t *src, size_t src_len,
                    uint8_t *dst,
                    const uint8_t iv[8]);

#endif
