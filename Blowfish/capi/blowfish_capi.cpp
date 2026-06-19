#define BLOWFISH_BUILD_DLL
#include "blowfish_capi.h"
#include "../algo/blowfish.h"

#include <cstdlib>
#include <cstring>
#include <vector>

// =============================================================================
//  Внутренний контекст — не виден снаружи библиотеки (struct, не class).
//  Аналог TEAContext из tea_capi.cpp и AESContext из aes_capi.cpp.
// =============================================================================

struct BlowfishContext {
    BFKey key;
};

// =============================================================================
//  Вспомогательная функция: скопировать vector<uint8_t> в malloc-буфер.
//  Используется и для encrypt, и для decrypt, чтобы не дублировать код.
// =============================================================================

static int copy_vector_to_malloc_buffer(const std::vector<uint8_t>& src,
                                         uint8_t** outData, size_t* outLen)
{
    uint8_t* buffer = static_cast<uint8_t*>(malloc(src.size()));
    if (buffer == nullptr) {
        *outData = nullptr;
        *outLen  = 0;
        return 0;
    }

    memcpy(buffer, src.data(), src.size());
    *outData = buffer;
    *outLen  = src.size();
    return 1;
}

// =============================================================================
//  Жизненный цикл
// =============================================================================

BlowfishHandle blowfish_create() {
    BlowfishContext* ctx = (BlowfishContext*)malloc(sizeof(BlowfishContext));
    if (ctx == nullptr) return nullptr;

    bf_key_init(&ctx->key);
    return ctx;
}

void blowfish_destroy(BlowfishHandle handle) {
    if (handle == nullptr) return;
    free(handle);
}

// =============================================================================
//  blowfish_set_key
// =============================================================================

int blowfish_set_key(BlowfishHandle handle,
                     const uint8_t* key, size_t keyLen)
{
    if (handle == nullptr) return 0;
    if (key    == nullptr) return 0;
    if (keyLen < BLOWFISH_KEY_MIN || keyLen > BLOWFISH_KEY_MAX) return 0;

    BlowfishContext* ctx = static_cast<BlowfishContext*>(handle);
    return bf_key_set(&ctx->key, key, (int)keyLen) ? 1 : 0;
}

// =============================================================================
//  blowfish_encrypt_cbc
// =============================================================================

int blowfish_encrypt_cbc(BlowfishHandle handle,
                          const uint8_t* data, size_t dataLen,
                          const uint8_t* iv,
                          uint8_t** outData, size_t* outLen)
{
    if (handle  == nullptr) return 0;
    if (data    == nullptr) return 0;
    if (iv      == nullptr) return 0;
    if (outData == nullptr) return 0;
    if (outLen  == nullptr) return 0;

    *outData = nullptr;
    *outLen  = 0;

    BlowfishContext* ctx = static_cast<BlowfishContext*>(handle);

    std::vector<uint8_t> plain(data, data + dataLen);
    std::vector<uint8_t> cipher = bf_cbc_encrypt(&ctx->key, plain, iv);

    if (cipher.empty()) return 0;

    return copy_vector_to_malloc_buffer(cipher, outData, outLen);
}

// =============================================================================
//  blowfish_decrypt_cbc
// =============================================================================

int blowfish_decrypt_cbc(BlowfishHandle handle,
                          const uint8_t* data, size_t dataLen,
                          const uint8_t* iv,
                          uint8_t** outData, size_t* outLen)
{
    if (handle  == nullptr) return 0;
    if (data    == nullptr) return 0;
    if (iv      == nullptr) return 0;
    if (outData == nullptr) return 0;
    if (outLen  == nullptr) return 0;

    *outData = nullptr;
    *outLen  = 0;

    BlowfishContext* ctx = static_cast<BlowfishContext*>(handle);

    std::vector<uint8_t> cipher(data, data + dataLen);
    std::vector<uint8_t> plain = bf_cbc_decrypt(&ctx->key, cipher, iv);

    if (plain.empty() && dataLen > 0) return 0;

    return copy_vector_to_malloc_buffer(plain, outData, outLen);
}

// =============================================================================
//  blowfish_free_buffer
// =============================================================================

void blowfish_free_buffer(uint8_t* buffer) {
    if (buffer != nullptr)
        free(buffer);
}