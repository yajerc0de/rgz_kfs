#ifndef RC5_H
#define RC5_H

#include <stdint.h>
#include <stddef.h>

/* RC5-32/12/16: word=32bit, rounds=12, keylen=16 bytes */
#define RC5_W        32
#define RC5_R        12
#define RC5_B        16
#define RC5_T        (2 * (RC5_R + 1))  /* size of expanded key table: 26 */

/* Magic constants for 32-bit words */
#define RC5_P32  0xB7E15163u
#define RC5_Q32  0x9E3779B9u

/* Rotate left/right macros for 32-bit values */
#define ROTL32(x, n)  (((x) << ((n) & 31)) | ((x) >> (32 - ((n) & 31))))
#define ROTR32(x, n)  (((x) >> ((n) & 31)) | ((x) << (32 - ((n) & 31))))

/* RC5 context: holds the expanded key table */
typedef struct {
    uint32_t S[RC5_T];
} RC5_CTX;

/* Initialize RC5 key schedule from raw key bytes */
void rc5_init(RC5_CTX *ctx, const uint8_t *key, size_t key_len);

/* Encrypt one 8-byte block (in-place, two 32-bit halves) */
void rc5_encrypt_block(const RC5_CTX *ctx, uint32_t *A, uint32_t *B);

/* Decrypt one 8-byte block (in-place) */
void rc5_decrypt_block(const RC5_CTX *ctx, uint32_t *A, uint32_t *B);

#endif /* RC5_H */
