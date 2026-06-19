#pragma once

#include "../../../core/loader.h"
#include "../capi/rc5_capi.h"

// =============================================================================
//  rc5_module_api.h — обёртка над DynamicLibrary специально для RC5
//
//  Загружает rc5.dll/rc5.so и достаёт оттуда все нужные функции через
//  GetProcAddress/dlsym, сохраняя их как указатели на функции внутри обычной
//  структуры (POD). Никаких методов с поведением — только данные и свободные
//  функции rc5ModuleLoad/rc5ModuleIsReady, которые их заполняют и проверяют.
// =============================================================================

// ── Сигнатуры функций — должны точно совпадать с rc5_capi.h ──────────────────

typedef Rc5Handle (*Rc5CreateFn)();
typedef void      (*Rc5DestroyFn)(Rc5Handle);
typedef int       (*Rc5SetKeyFn)(Rc5Handle, const uint8_t*, size_t);
typedef int       (*Rc5EncryptCbcFn)(Rc5Handle, const uint8_t*, size_t,
                                      const uint8_t*, uint8_t**, size_t*);
typedef int       (*Rc5DecryptCbcFn)(Rc5Handle, const uint8_t*, size_t,
                                      const uint8_t*, uint8_t**, size_t*);
typedef void      (*Rc5FreeBufferFn)(uint8_t*);

// ── Структура с указателями на функции и состоянием библиотеки ───────────────
// Обычная структура данных — без конструкторов и методов.

struct Rc5Module {
    DynamicLibrary lib;
    string         lastError;

    Rc5CreateFn      create     = nullptr;
    Rc5DestroyFn     destroy    = nullptr;
    Rc5SetKeyFn      setKey     = nullptr;
    Rc5EncryptCbcFn  encryptCbc = nullptr;
    Rc5DecryptCbcFn  decryptCbc = nullptr;
    Rc5FreeBufferFn  freeBuffer = nullptr;
};

// ── Свободные функции для работы с Rc5Module ──────────────────────────────────

// Загрузить библиотеку по пути и достать все символы в mod.
// Возвращает false если библиотека не найдена или каких-то функций нет внутри.
bool rc5ModuleLoad(Rc5Module& mod, const string& libPath);

// Загружена ли библиотека и все её функции успешно найдены.
bool rc5ModuleIsReady(const Rc5Module& mod);
