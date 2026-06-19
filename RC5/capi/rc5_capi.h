#pragma once

#include <cstdint>
#include <cstddef>

// =============================================================================
//  rc5_capi.h — C-совместимый интерфейс модуля RC5
//
//  Handle — это указатель на Rc5KeySchedule (обычная структура без методов),
//  выделенную malloc-ом внутри библиотеки. Снаружи он непрозрачен — просто void*.
// =============================================================================

#ifdef _WIN32
    #ifdef RC5_BUILD_DLL
        #define RC5_API __declspec(dllexport)
    #else
        #define RC5_API __declspec(dllimport)
    #endif
#else
    #define RC5_API __attribute__((visibility("default")))
#endif

extern "C" {

typedef void* Rc5Handle;

#define RC5_BLOCK_BYTES 8
#define RC5_KEY_BYTES   16

// ── Жизненный цикл ────────────────────────────────────────────────────────────

// Создать новое ключевое расписание RC5. Возвращает handle или nullptr при ошибке.
// Каждый созданный handle должен быть освобождён через rc5_destroy().
RC5_API Rc5Handle rc5_create();

// Освободить структуру, созданную rc5_create().
RC5_API void rc5_destroy(Rc5Handle handle);

// Установить ключ (от 1 до 255 байт; рекомендуется 16 байт = 128 бит).
// Возвращает 1 при успехе, 0 при неверной длине ключа.
RC5_API int rc5_set_key(Rc5Handle handle, const uint8_t* key, size_t keyLen);

// ── CBC-шифрование / дешифрование ───────────────────────────────────────────
//
// Память под outData выделяется внутри библиотеки (malloc) — после
// использования результата вызывающая сторона ОБЯЗАНА вызвать rc5_free_buffer.

RC5_API int rc5_encrypt_cbc(Rc5Handle handle,
                             const uint8_t* data, size_t dataLen,
                             const uint8_t* iv,
                             uint8_t** outData, size_t* outLen);

RC5_API int rc5_decrypt_cbc(Rc5Handle handle,
                             const uint8_t* data, size_t dataLen,
                             const uint8_t* iv,
                             uint8_t** outData, size_t* outLen);

// Освободить буфер, полученный из rc5_encrypt_cbc / rc5_decrypt_cbc.
RC5_API void rc5_free_buffer(uint8_t* buffer);

} // extern "C"
