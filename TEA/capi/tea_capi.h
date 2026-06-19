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

// Размеры, фиксированные алгоритмом TEA
#define TEA_BLOCK_BYTES 8
#define TEA_KEY_BYTES   16

// ── Жизненный цикл ────────────────────────────────────────────────────────────

// Создать новый контекст TEA. Возвращает handle или nullptr при ошибке.
// Каждый созданный handle должен быть освобождён через tea_destroy().
TEA_API TEAHandle tea_create();

// Освободить контекст, созданный tea_create().
TEA_API void tea_destroy(TEAHandle handle);

// Установить ключ (ровно TEA_KEY_BYTES = 16 байт).
// Возвращает 1 при успехе, 0 при неверной длине ключа или nullptr.
TEA_API int tea_set_key(TEAHandle handle, const uint8_t* key, size_t keyLen);

// ── CBC-шифрование / дешифрование ─────────────────────────────────────────────
//
// Память под outData выделяется внутри библиотеки (malloc) — после
// использования результата вызывающая сторона ОБЯЗАНА вызвать tea_free_buffer.

// Зашифровать data (dataLen байт) в режиме CBC + PKCS#7.
// iv должен быть ровно TEA_BLOCK_BYTES байт.
// При успехе: возвращает 1, *outData — новый буфер, *outLen — его длина.
// При ошибке: возвращает 0, *outData = nullptr.
TEA_API int tea_encrypt_cbc(TEAHandle handle,
                             const uint8_t* data, size_t dataLen,
                             const uint8_t* iv,
                             uint8_t** outData, size_t* outLen);

// Расшифровать data (dataLen байт) в режиме CBC, снять PKCS#7 паддинг.
// iv должен быть ровно TEA_BLOCK_BYTES байт.
// При ошибке (неверный ключ, повреждённые данные, неверный паддинг): возвращает 0.
TEA_API int tea_decrypt_cbc(TEAHandle handle,
                             const uint8_t* data, size_t dataLen,
                             const uint8_t* iv,
                             uint8_t** outData, size_t* outLen);

// Освободить буфер, полученный из tea_encrypt_cbc / tea_decrypt_cbc.
TEA_API void tea_free_buffer(uint8_t* buffer);

} // extern "C"