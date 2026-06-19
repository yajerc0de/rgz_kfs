#pragma once

#include <cstdint>
#include <cstddef>

// ── Экспорт символов ──────────────────────────────────────────────────────────
#ifdef _WIN32
    #ifdef BLOWFISH_BUILD_DLL
        #define BLOWFISH_API __declspec(dllexport)
    #else
        #define BLOWFISH_API __declspec(dllimport)
    #endif
#else
    #define BLOWFISH_API __attribute__((visibility("default")))
#endif

extern "C" {

// ── Непрозрачный указатель на внутренний контекст Blowfish ────────────────────
typedef void* BlowfishHandle;

// Размеры, фиксированные алгоритмом Blowfish
#define BLOWFISH_BLOCK_BYTES 8
#define BLOWFISH_KEY_MIN     4
#define BLOWFISH_KEY_MAX     56

// ── Жизненный цикл ────────────────────────────────────────────────────────────

// Создать новый контекст Blowfish. Возвращает handle или nullptr при ошибке.
// Каждый созданный handle должен быть освобождён через blowfish_destroy().
BLOWFISH_API BlowfishHandle blowfish_create();

// Освободить контекст, созданный blowfish_create().
BLOWFISH_API void blowfish_destroy(BlowfishHandle handle);

// Установить ключ (длина от BLOWFISH_KEY_MIN до BLOWFISH_KEY_MAX байт).
// Запускает Key Schedule. Возвращает 1 при успехе, 0 при неверной длине.
BLOWFISH_API int blowfish_set_key(BlowfishHandle handle,
                                   const uint8_t* key, size_t keyLen);

// ── CBC-шифрование / дешифрование ─────────────────────────────────────────────
//
// Память под outData выделяется внутри библиотеки (malloc) — после
// использования результата вызывающая сторона ОБЯЗАНА вызвать
// blowfish_free_buffer.

// Зашифровать data (dataLen байт) в режиме CBC + PKCS#7.
// iv должен быть ровно BLOWFISH_BLOCK_BYTES байт.
// При успехе: возвращает 1, *outData — новый буфер, *outLen — его длина.
// При ошибке: возвращает 0, *outData = nullptr.
BLOWFISH_API int blowfish_encrypt_cbc(BlowfishHandle handle,
                                       const uint8_t* data, size_t dataLen,
                                       const uint8_t* iv,
                                       uint8_t** outData, size_t* outLen);

// Расшифровать data (dataLen байт) в режиме CBC, снять PKCS#7 паддинг.
// iv должен быть ровно BLOWFISH_BLOCK_BYTES байт.
// При ошибке (неверный ключ, повреждённые данные, неверный паддинг): возвращает 0.
BLOWFISH_API int blowfish_decrypt_cbc(BlowfishHandle handle,
                                       const uint8_t* data, size_t dataLen,
                                       const uint8_t* iv,
                                       uint8_t** outData, size_t* outLen);

// Освободить буфер, полученный из blowfish_encrypt_cbc / blowfish_decrypt_cbc.
BLOWFISH_API void blowfish_free_buffer(uint8_t* buffer);

} // extern "C"