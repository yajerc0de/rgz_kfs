#include "rc5.h"
#include <string.h>

void rc5_init(RC5_CTX *ctx, const uint8_t *key, size_t key_len)
{
    uint32_t L[RC5_B / 4];  
    int i, j, k;
    uint32_t A, B;

   
    memset(L, 0, sizeof(L));
    for (i = (int)key_len - 1; i >= 0; i--)
        L[i / 4] = (L[i / 4] << 8) | key[i];

 
    ctx->S[0] = RC5_P32;
    for (i = 1; i < RC5_T; i++)
        ctx->S[i] = ctx->S[i - 1] + RC5_Q32;

   
    A = B = 0;
    i = j = 0;
    k = 3 * RC5_T;   
    if (3 * (RC5_B / 4) > k)
        k = 3 * (RC5_B / 4);

    for (; k > 0; k--) {
        ctx->S[i] = ROTL32(ctx->S[i] + A + B, 3);
        A = ctx->S[i];
        i = (i + 1) % RC5_T;

        L[j] = ROTL32(L[j] + A + B, A + B);
        B = L[j];
        j = (j + 1) % (RC5_B / 4);
    }
}


void rc5_encrypt_block(const RC5_CTX *ctx, uint32_t *A, uint32_t *B)
{
    int i;
    *A = *A + ctx->S[0];
    *B = *B + ctx->S[1];

    for (i = 1; i <= RC5_R; i++) {
        *A = ROTL32(*A ^ *B, *B) + ctx->S[2 * i];
        *B = ROTL32(*B ^ *A, *A) + ctx->S[2 * i + 1];
    }
}

void rc5_decrypt_block(const RC5_CTX *ctx, uint32_t *A, uint32_t *B)
{
    int i;
    for (i = RC5_R; i >= 1; i--) {
        *B = ROTR32(*B - ctx->S[2 * i + 1], *A) ^ *A;
        *A = ROTR32(*A - ctx->S[2 * i],     *B) ^ *B;
    }
    *B = *B - ctx->S[1];
    *A = *A - ctx->S[0];
}
