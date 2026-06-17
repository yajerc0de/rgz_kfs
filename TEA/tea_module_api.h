#pragma once

#include "loader.h"
#include "tea_capi.h"

// =============================================================================
//  tea_module_api.h — обёртка над DynamicLibrary специально для TEA
//
//  Загружает tea.dll/tea.so и достаёт оттуда все нужные функции через
//  GetProcAddress/dlsym, сохраняя их как указатели на функции.
//  tea_ui.cpp использует ТОЛЬКО этот класс — никакого прямого знания
//  о классе TEA или способе его реализации.
// =============================================================================

// ── Сигнатуры функций — должны точно совпадать с tea_capi.h ──────────────────

typedef TeaHandle (*TeaCreateFn)();
typedef void      (*TeaDestroyFn)(TeaHandle);
typedef int       (*TeaSetKeyFn)(TeaHandle, const uint8_t*, size_t);
typedef int       (*TeaEncryptCbcFn)(TeaHandle, const uint8_t*, size_t,
                                      const uint8_t*, uint8_t**, size_t*);
typedef int       (*TeaDecryptCbcFn)(TeaHandle, const uint8_t*, size_t,
                                      const uint8_t*, uint8_t**, size_t*);
typedef void      (*TeaFreeBufferFn)(uint8_t*);

class TeaModule {
public:
    // Загрузить библиотеку по пути и достать все символы.
    // Возвращает false если библиотека не найдена или каких-то функций нет внутри.
    bool load(const string& libPath);

    // Загружена ли библиотека и все её функции успешно найдены.
    bool isReady() const;

    // Текст последней ошибки.
    const string& lastError() const;

    // ── Указатели на функции — вызываются напрямую как tea.create() и т.п. ────
    TeaCreateFn      create      = nullptr;
    TeaDestroyFn     destroy     = nullptr;
    TeaSetKeyFn      setKey      = nullptr;
    TeaEncryptCbcFn  encryptCbc  = nullptr;
    TeaDecryptCbcFn  decryptCbc  = nullptr;
    TeaFreeBufferFn  freeBuffer  = nullptr;

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
bool TeaModule::bindSymbol(const string& name, FnPtr& target) {
    void* sym = m_lib.getSymbol(name);
    if (sym == nullptr) {
        m_lastError = "Не найден символ в библиотеке: " + name;
        return false;
    }
    target = reinterpret_cast<FnPtr>(sym);
    return true;
}