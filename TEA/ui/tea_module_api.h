#pragma once

#include "../../core/loader.h"
#include "../capi/tea_capi.h"

#include <string>

// ── Сигнатуры функций — должны точно совпадать с tea_capi.h ──────────────────

typedef TEAHandle (*TEACreateFn)();
typedef void      (*TEADestroyFn)(TEAHandle);
typedef int       (*TEASetKeyFn)(TEAHandle, const uint8_t*, size_t);
typedef int       (*TEAEncryptCbcFn)(TEAHandle, const uint8_t*, size_t,
                                      const uint8_t*, uint8_t**, size_t*);
typedef int       (*TEADecryptCbcFn)(TEAHandle, const uint8_t*, size_t,
                                      const uint8_t*, uint8_t**, size_t*);
typedef void      (*TEAFreeBufferFn)(uint8_t*);

// =============================================================================
//  TEAModule — struct с указателями на функции из tea.dll / tea.so.
//  Аналог AESModule из aes_module_api.h.
//  Никакой логики внутри struct — только данные.
// =============================================================================

struct TEAModule {
    DynamicLibrary library;

    TEACreateFn     create     = nullptr;
    TEADestroyFn    destroy    = nullptr;
    TEASetKeyFn     setKey     = nullptr;
    TEAEncryptCbcFn encryptCbc = nullptr;
    TEADecryptCbcFn decryptCbc = nullptr;
    TEAFreeBufferFn freeBuffer = nullptr;

    std::string lastError;
};

// ── Свободные функции для работы с TEAModule ─────────────────────────────────

// Загрузить библиотеку по пути и привязать все символы.
// Возвращает false если библиотека не найдена или символ отсутствует.
bool tea_load(TEAModule* module, const std::string& path);

// Все указатели заполнены и готовы к использованию.
bool tea_is_ready(const TEAModule* module);

// Текст последней ошибки.
const std::string& tea_last_error(const TEAModule* module);

// ── Шаблонная вспомогательная функция привязки символа ───────────────────────
// Реализация в заголовке — как в aes_module_api.h.

template<typename FnPtr>
bool tea_bind_symbol(TEAModule* module, const std::string& name, FnPtr& target) {
    void* sym = module->library.getSymbol(name);

    if (sym == nullptr) {
        module->lastError = "Не найден символ: " + name;
        return false;
    }

    target = reinterpret_cast<FnPtr>(sym);
    return true;
}