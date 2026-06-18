#pragma once

#include <cstdint>
#include <cstddef>

// =============================================================================
//  tea_capi.h — C-совместимый интерфейс модуля TEA
//
//  Этот заголовок — единственная точка контакта между .dll/.so и остальной
//  программой. Класс TEA (C++) нельзя безопасно передавать через границу
//  динамической библиотеки — ABI разных компиляторов несовместим.
//  Поэтому здесь только функции extern "C" и непрозрачный указатель (handle).
// =============================================================================

// ── Экспорт символов ──────────────────────────────────────────────────────────
// На Windows функции .dll нужно явно помечать __declspec(dllexport) при сборке
// самой библиотеки и __declspec(dllimport) при использовании из .exe.
// TEA_BUILD_DLL определяется только в проекте самой библиотеки (tea_capi.cpp).

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

// ── Непрозрачный указатель на объект TEA ──────────────────────────────────────
// Снаружи библиотеки никто не знает что внутри — просто handle.
typedef void* TeaHandle;

// Размеры, фиксированные алгоритмом TEA (совпадают с TEA::BLOCK_BYTES / KEY_BYTES)
#define TEA_BLOCK_BYTES 8
#define TEA_KEY_BYTES   16

// ── Жизненный цикл ────────────────────────────────────────────────────────────

// Создать новый экземпляр TEA. Возвращает handle или nullptr при ошибке.
// Каждый созданный handle должен быть освобождён через tea_destroy().
TEA_API TeaHandle tea_create();

// Освободить объект TEA, созданный tea_create().
TEA_API void tea_destroy(TeaHandle handle);

// Установить ключ (ровно TEA_KEY_BYTES = 16 байт).
// Возвращает 1 при успехе, 0 при неверной длине ключа.
TEA_API int tea_set_key(TeaHandle handle, const uint8_t* key, size_t keyLen);

// ── CBC-шифрование / дешифрование ───────────────────────────────────────────
//
// Память под outData выделяется внутри библиотеки (malloc) — после
// использования результата вызывающая сторона ОБЯЗАНА вызвать tea_free_buffer.
// Это необходимо, так как .exe и .dll могут использовать разные runtime-кучи.

// Зашифровать data (dataLen байт) в режиме CBC.
// iv должен быть ровно TEA_BLOCK_BYTES байт.
// При успехе: возвращает 1, *outData указывает на новый буфер, *outLen — его длина.
// При ошибке: возвращает 0, *outData = nullptr.
TEA_API int tea_encrypt_cbc(TeaHandle handle,
                             const uint8_t* data, size_t dataLen,
                             const uint8_t* iv,
                             uint8_t** outData, size_t* outLen);

// Расшифровать data (dataLen байт) в режиме CBC, снять PKCS#7 паддинг.
// iv должен быть ровно TEA_BLOCK_BYTES байт.
// При успехе: возвращает 1, *outData указывает на новый буфер, *outLen — его длина.
// При ошибке (неверный ключ, повреждённые данные, неверный паддинг): возвращает 0.
TEA_API int tea_decrypt_cbc(TeaHandle handle,
                             const uint8_t* data, size_t dataLen,
                             const uint8_t* iv,
                             uint8_t** outData, size_t* outLen);

// Освободить буфер, полученный из tea_encrypt_cbc / tea_decrypt_cbc.
TEA_API void tea_free_buffer(uint8_t* buffer);

} // extern "C"