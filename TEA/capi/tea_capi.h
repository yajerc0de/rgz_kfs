#pragma once

#include <cstdint>
#include <cstddef>

// ── Экспорт символов ──────────────────────────────────────────────────────────
#ifdef _WIN32
    #ifdef TEA_BUILD_DLL
        #define TEA_API __declspec(dllexport)
    #else
        #define TEA_API __declspec(dllimport)
    #endif
#else
    #define TEA_API __attribute__((visibility("default")))
#endif

extern "C" {

// ── Непрозрачный указатель на внутренний контекст TEA ─────────────────────────
typedef void* TEAHandle;

// Размеры для использования в ui-слое.
// Названы с суффиксом _CAPI чтобы не конфликтовать с constexpr-константами
// TEA_KEY_BYTES / TEA_BLOCK_BYTES_ALGO из algo/tea.h.
#define TEA_CAPI_BLOCK_BYTES 8
#define TEA_CAPI_KEY_BYTES   16

// ── Жизненный цикл ────────────────────────────────────────────────────────────

TEA_API TEAHandle tea_create();
TEA_API void      tea_destroy(TEAHandle handle);

// Установить ключ (ровно TEA_CAPI_KEY_BYTES = 16 байт).
// Возвращает 1 при успехе, 0 при ошибке.
TEA_API int tea_set_key(TEAHandle handle, const uint8_t* key, size_t keyLen);

// ── CBC-шифрование / дешифрование ─────────────────────────────────────────────
// iv должен быть ровно TEA_CAPI_BLOCK_BYTES байт.
// Память под outData выделяется через malloc — освободить через tea_free_buffer.

TEA_API int tea_encrypt_cbc(TEAHandle handle,
                             const uint8_t* data, size_t dataLen,
                             const uint8_t* iv,
                             uint8_t** outData, size_t* outLen);

TEA_API int tea_decrypt_cbc(TEAHandle handle,
                             const uint8_t* data, size_t dataLen,
                             const uint8_t* iv,
                             uint8_t** outData, size_t* outLen);

TEA_API void tea_free_buffer(uint8_t* buffer);

} // extern "C"