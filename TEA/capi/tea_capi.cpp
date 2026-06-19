#define TEA_BUILD_DLL
#include "tea_capi.h"
#include "../algo/tea.h"

#include <cstdlib>
#include <cstring>
#include <vector>

// =============================================================================
//  Внутренний контекст — не виден снаружи библиотеки (struct, не class).
//  Аналог AESContext из aes_capi.cpp.
// =============================================================================

struct TEAContext {
    TeaKey key;
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

TEAHandle tea_create() {
    TEAContext* ctx = (TEAContext*)malloc(sizeof(TEAContext));
    if (ctx == nullptr) return nullptr;

    tea_key_init(&ctx->key);
    return ctx;
}

void tea_destroy(TEAHandle handle) {
    if (handle == nullptr) return;
    free(handle);
}

// =============================================================================
//  tea_set_key
// =============================================================================

int tea_set_key(TEAHandle handle, const uint8_t* key, size_t keyLen) {
    if (handle == nullptr) return 0;
    if (key == nullptr)    return 0;
    if (keyLen != TEA_KEY_BYTES) return 0;

    TEAContext* ctx = static_cast<TEAContext*>(handle);
    return tea_key_set(&ctx->key, key, (int)keyLen) ? 1 : 0;
}

// =============================================================================
//  tea_encrypt_cbc
// =============================================================================

int tea_encrypt_cbc(TEAHandle handle,
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

    TEAContext* ctx = static_cast<TEAContext*>(handle);

    std::vector<uint8_t> plain(data, data + dataLen);
    std::vector<uint8_t> cipher = tea_cbc_encrypt(&ctx->key, plain, iv);

    if (cipher.empty()) return 0;

    return copy_vector_to_malloc_buffer(cipher, outData, outLen);
}

// =============================================================================
//  tea_decrypt_cbc
// =============================================================================

int tea_decrypt_cbc(TEAHandle handle,
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

    TEAContext* ctx = static_cast<TEAContext*>(handle);

    std::vector<uint8_t> cipher(data, data + dataLen);
    std::vector<uint8_t> plain = tea_cbc_decrypt(&ctx->key, cipher, iv);

    if (plain.empty() && dataLen > 0) return 0;

    return copy_vector_to_malloc_buffer(plain, outData, outLen);
}

// =============================================================================
//  tea_free_buffer
// =============================================================================

void tea_free_buffer(uint8_t* buffer) {
    if (buffer != nullptr)
        free(buffer);
}