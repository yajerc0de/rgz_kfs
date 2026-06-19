#pragma once

#include "../../core/loader.h"
#include "../capi/blowfish_capi.h"

#include <string>

// ── Сигнатуры функций — должны точно совпадать с blowfish_capi.h ──────────────

typedef BlowfishHandle (*BlowfishCreateFn)();
typedef void           (*BlowfishDestroyFn)(BlowfishHandle);
typedef int            (*BlowfishSetKeyFn)(BlowfishHandle, const uint8_t*, size_t);
typedef int            (*BlowfishEncryptCbcFn)(BlowfishHandle, const uint8_t*, size_t,
                                                const uint8_t*, uint8_t**, size_t*);
typedef int            (*BlowfishDecryptCbcFn)(BlowfishHandle, const uint8_t*, size_t,
                                                const uint8_t*, uint8_t**, size_t*);
typedef void           (*BlowfishFreeBufferFn)(uint8_t*);

// =============================================================================
//  BlowfishModule — struct с указателями на функции из blowfish.dll / blowfish.so.
//  Аналог AESModule из aes_module_api.h и TEAModule из tea_module_api.h.
//  Никакой логики внутри struct — только данные.
// =============================================================================

struct BlowfishModule {
    DynamicLibrary library;

    BlowfishCreateFn     create     = nullptr;
    BlowfishDestroyFn    destroy    = nullptr;
    BlowfishSetKeyFn     setKey     = nullptr;
    BlowfishEncryptCbcFn encryptCbc = nullptr;
    BlowfishDecryptCbcFn decryptCbc = nullptr;
    BlowfishFreeBufferFn freeBuffer = nullptr;

    std::string lastError;
};

// ── Свободные функции для работы с BlowfishModule ────────────────────────────

// Загрузить библиотеку по пути и привязать все символы.
// Возвращает false если библиотека не найдена или символ отсутствует.
bool blowfish_load(BlowfishModule* module, const std::string& path);

// Все указатели заполнены и готовы к использованию.
bool blowfish_is_ready(const BlowfishModule* module);

// Текст последней ошибки.
const std::string& blowfish_last_error(const BlowfishModule* module);

// ── Шаблонная вспомогательная функция привязки символа ───────────────────────
// Реализация в заголовке — как в aes_module_api.h и tea_module_api.h.

template<typename FnPtr>
bool blowfish_bind_symbol(BlowfishModule* module, const std::string& name, FnPtr& target) {
    void* sym = module->library.getSymbol(name);

    if (sym == nullptr) {
        module->lastError = "Не найден символ: " + name;
        return false;
    }

    target = reinterpret_cast<FnPtr>(sym);
    return true;
}