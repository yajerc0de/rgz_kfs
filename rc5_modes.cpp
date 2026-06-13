#include "rc5_modes.h"
#include <string.h>

// Читаем 4 байта в uint32_t (little-endian) 
static uint32_t load32le(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

// Записываем uint32_t в 4 байта (little-endian) 
static void store32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

int rc5_cbc_encrypt(const RC5_CTX *ctx,
                    const uint8_t *src, size_t src_len,
                    uint8_t *dst,
                    const uint8_t iv[8])
{
    uint8_t block[8];
    uint8_t prev[8];
    uint32_t A, B;
    size_t offset;
    int i, out_len, pad;
    size_t num_full;

    memcpy(prev, iv, 8);
    num_full = src_len / 8;
    out_len  = 0;

    // Шифруем полные блоки по 8 байт 
    for (offset = 0; offset < num_full * 8; offset += 8) {
        for (i = 0; i < 8; i++)
            block[i] = src[offset + i] ^ prev[i];

        A = load32le(block);
        B = load32le(block + 4);
        rc5_encrypt_block(ctx, &A, &B);
        store32le(dst + out_len,     A);
        store32le(dst + out_len + 4, B);

        memcpy(prev, dst + out_len, 8);
        out_len += 8;
    }

    // Последний блок с набивкой PKCS#7
    pad = 8 - (int)(src_len % 8);
    memcpy(block, src + num_full * 8, src_len % 8);
    for (i = (int)(src_len % 8); i < 8; i++)
        block[i] = (uint8_t)pad;

    for (i = 0; i < 8; i++)
        block[i] ^= prev[i];

    A = load32le(block);
    B = load32le(block + 4);
    rc5_encrypt_block(ctx, &A, &B);
    store32le(dst + out_len,     A);
    store32le(dst + out_len + 4, B);
    out_len += 8;

    return out_len;
}

int rc5_cbc_decrypt(const RC5_CTX *ctx,
                    const uint8_t *src, size_t src_len,
                    uint8_t *dst,
                    const uint8_t iv[8])
{
    uint8_t block[8];
    uint8_t prev[8];
    uint8_t cur[8];
    uint32_t A, B;
    size_t offset;
    int i, out_len, pad;

    if (src_len == 0 || src_len % 8 != 0)
        return -1;

    memcpy(prev, iv, 8);
    out_len = 0;

    for (offset = 0; offset < src_len; offset += 8) {
        memcpy(cur, src + offset, 8);

        A = load32le(cur);
        B = load32le(cur + 4);
        rc5_decrypt_block(ctx, &A, &B);
        store32le(block,     A);
        store32le(block + 4, B);

        for (i = 0; i < 8; i++)
            dst[out_len + i] = block[i] ^ prev[i];

        memcpy(prev, cur, 8);
        out_len += 8;
    }

    
    pad = dst[out_len - 1];
    if (pad < 1 || pad > 8)
        return -1;
    for (i = 0; i < pad; i++) {
        if (dst[out_len - 1 - i] != (uint8_t)pad)
            return -1;
    }
    out_len -= pad;

    return out_len;
}
