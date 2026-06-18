#pragma once

#include "../../core/loader.h"
#include "blowfish_capi.h"

// =============================================================================
//  blowfish_module_api.h — обёртка над DynamicLibrary специально для Blowfish
//
//  Загружает blowfish.dll/blowfish.so и достаёт оттуда все нужные функции
//  через GetProcAddress/dlsym, сохраняя их как указатели на функции.
//  blowfish_ui.cpp использует ТОЛЬКО этот класс — никакого прямого знания
//  о классе Blowfish или способе его реализации.
// =============================================================================

// ── Сигнатуры функций — должны точно совпадать с blowfish_capi.h ─────────────

typedef BlowfishHandle (*BlowfishCreateFn)();
typedef void           (*BlowfishDestroyFn)(BlowfishHandle);
typedef int             (*BlowfishSetKeyFn)(BlowfishHandle, const uint8_t*, size_t);
typedef int             (*BlowfishEncryptCbcFn)(BlowfishHandle, const uint8_t*, size_t,
                                                 const uint8_t*, uint8_t**, size_t*);
typedef int             (*BlowfishDecryptCbcFn)(BlowfishHandle, const uint8_t*, size_t,
                                                 const uint8_t*, uint8_t**, size_t*);
typedef void            (*BlowfishFreeBufferFn)(uint8_t*);

class BlowfishModule {
public:
    // Загрузить библиотеку по пути и достать все символы.
    // Возвращает false если библиотека не найдена или каких-то функций нет внутри.
    bool load(const string& libPath);

    // Загружена ли библиотека и все её функции успешно найдены.
    bool isReady() const;

    // Текст последней ошибки.
    const string& lastError() const;

    // ── Указатели на функции — вызываются напрямую как mod.create() и т.п. ────
    BlowfishCreateFn      create      = nullptr;
    BlowfishDestroyFn     destroy     = nullptr;
    BlowfishSetKeyFn      setKey      = nullptr;
    BlowfishEncryptCbcFn  encryptCbc  = nullptr;
    BlowfishDecryptCbcFn  decryptCbc  = nullptr;
    BlowfishFreeBufferFn  freeBuffer  = nullptr;

private:
    DynamicLibrary m_lib;
    string         m_lastError;

    // Достать один символ и привести к нужному типу функции.
    // Возвращает false и заполняет m_lastError если символ не найден.
    template <typename FnPtr>
    bool bindSymbol(const string& name, FnPtr& target);
};

// ── Реализация шаблонного метода — должна быть в заголовке ───────────────────

template <typename FnPtr>
bool BlowfishModule::bindSymbol(const string& name, FnPtr& target) {
    void* sym = m_lib.getSymbol(name);
    if (sym == nullptr) {
        m_lastError = "Не найден символ в библиотеке: " + name;
        return false;
    }
    target = reinterpret_cast<FnPtr>(sym);
    return true;
}