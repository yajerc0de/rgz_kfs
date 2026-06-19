#include "tea_module_api.h"

// =============================================================================
//  tea_load — загрузить библиотеку и привязать все символы
// =============================================================================

bool tea_load(TEAModule* module, const std::string& path) {
    if (module == nullptr)
        return false;

    if (!module->library.load(path)) {
        module->lastError = module->library.lastError();
        return false;
    }

    bool ok = true;

    ok = ok && tea_bind_symbol(module, "tea_create",      module->create);
    ok = ok && tea_bind_symbol(module, "tea_destroy",     module->destroy);
    ok = ok && tea_bind_symbol(module, "tea_set_key",     module->setKey);
    ok = ok && tea_bind_symbol(module, "tea_encrypt_cbc", module->encryptCbc);
    ok = ok && tea_bind_symbol(module, "tea_decrypt_cbc", module->decryptCbc);
    ok = ok && tea_bind_symbol(module, "tea_free_buffer", module->freeBuffer);

    if (!ok) {
        module->library.unload();
        return false;
    }

    return true;
}

// =============================================================================
//  tea_is_ready — все указатели заполнены
// =============================================================================

bool tea_is_ready(const TEAModule* module) {
    if (module == nullptr)
        return false;

    return module->create     != nullptr &&
           module->destroy    != nullptr &&
           module->setKey     != nullptr &&
           module->encryptCbc != nullptr &&
           module->decryptCbc != nullptr &&
           module->freeBuffer != nullptr;
}

// =============================================================================
//  tea_last_error — текст последней ошибки
// =============================================================================

const std::string& tea_last_error(const TEAModule* module) {
    return module->lastError;
}