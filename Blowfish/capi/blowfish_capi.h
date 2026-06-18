#pragma once

#include <cstdint>
#include <cstddef>

// =============================================================================
//  blowfish_capi.h — C-совместимый интерфейс модуля Blowfish
//
//  Этот заголовок — единственная точка контакта между .dll/.so и остальной
//  программой. Класс Blowfish (C++) нельзя безопасно передавать через границу
//  динамической библиотеки — ABI разных компиляторов несовместим.
//  Поэтому здесь только функции extern "C" и непрозрачный указатель (handle).
// =============================================================================

// ── Экспорт символов ──────────────────────────────────────────────────────────
// На Windows функции .dll нужно явно помечать __declspec(dllexport) при сборке
// самой библиотеки и __declspec(dllimport) при использовании из .exe.
// BLOWFISH_BUILD_DLL определяется только в проекте самой библиотеки
// (blowfish_capi.cpp / CMakeLists.txt).

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

// ── Непрозрачный указатель на объект Blowfish ─────────────────────────────────
typedef void* BlowfishHandle;

// Размеры, фиксированные алгоритмом (совпадают с Blowfish::BLOCK_BYTES и т.д.)
#define BLOWFISH_BLOCK_BYTES 8
#define BLOWFISH_KEY_MIN     4
#define BLOWFISH_KEY_MAX     56

// ── Жизненный цикл ────────────────────────────────────────────────────────────

// Создать новый экземпляр Blowfish. Возвращает handle или nullptr при ошибке.
// Каждый созданный handle должен быть освобождён через blowfish_destroy().
BLOWFISH_API BlowfishHandle blowfish_create();

// Освободить объект Blowfish, созданный blowfish_create().
BLOWFISH_API void blowfish_destroy(BlowfishHandle handle);

// Установить ключ (длина от BLOWFISH_KEY_MIN до BLOWFISH_KEY_MAX байт).
// Запускает внутри Key Schedule. Возвращает 1 при успехе, 0 при неверной длине.
BLOWFISH_API int blowfish_set_key(BlowfishHandle handle,
                                   const uint8_t* key, size_t keyLen);

// ── CBC-шифрование / дешифрование ───────────────────────────────────────────
//
// Память под outData выделяется внутри библиотеки (malloc) — после
// использования результата вызывающая сторона ОБЯЗАНА вызвать
// blowfish_free_buffer. Это необходимо, так как .exe и .dll могут
// использовать разные runtime-кучи.

// Зашифровать data (dataLen байт) в режиме CBC.
// iv должен быть ровно BLOWFISH_BLOCK_BYTES байт.
// При успехе: возвращает 1, *outData указывает на новый буфер, *outLen — длина.
// При ошибке: возвращает 0, *outData = nullptr.
BLOWFISH_API int blowfish_encrypt_cbc(BlowfishHandle handle,
                                       const uint8_t* data, size_t dataLen,
                                       const uint8_t* iv,
                                       uint8_t** outData, size_t* outLen);

// Расшифровать data (dataLen байт) в режиме CBC, снять PKCS#7 паддинг.
// iv должен быть ровно BLOWFISH_BLOCK_BYTES байт.
// При успехе: возвращает 1. При ошибке (неверный ключ, повреждённые данные,
// неверный паддинг): возвращает 0.
BLOWFISH_API int blowfish_decrypt_cbc(BlowfishHandle handle,
                                       const uint8_t* data, size_t dataLen,
                                       const uint8_t* iv,
                                       uint8_t** outData, size_t* outLen);

// Освободить буфер, полученный из blowfish_encrypt_cbc / blowfish_decrypt_cbc.
BLOWFISH_API void blowfish_free_buffer(uint8_t* buffer);

} // extern "C"