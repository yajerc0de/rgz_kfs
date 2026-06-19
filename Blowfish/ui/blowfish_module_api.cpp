#include "blowfish_module_api.h"

// =============================================================================
//  blowfish_load — загрузить библиотеку и привязать все символы
// =============================================================================

bool blowfish_load(BlowfishModule* module, const std::string& path) {
    if (module == nullptr)
        return false;

    if (!module->library.load(path)) {
        module->lastError = module->library.lastError();
        return false;
    }

    bool ok = true;

    ok = ok && blowfish_bind_symbol(module, "blowfish_create",      module->create);
    ok = ok && blowfish_bind_symbol(module, "blowfish_destroy",     module->destroy);
    ok = ok && blowfish_bind_symbol(module, "blowfish_set_key",     module->setKey);
    ok = ok && blowfish_bind_symbol(module, "blowfish_encrypt_cbc", module->encryptCbc);
    ok = ok && blowfish_bind_symbol(module, "blowfish_decrypt_cbc", module->decryptCbc);
    ok = ok && blowfish_bind_symbol(module, "blowfish_free_buffer", module->freeBuffer);

    if (!ok) {
        module->library.unload();
        return false;
    }

    return true;
}

// =============================================================================
//  blowfish_is_ready — все указатели заполнены
// =============================================================================

bool blowfish_is_ready(const BlowfishModule* module) {
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
//  blowfish_last_error — текст последней ошибки
// =============================================================================

const std::string& blowfish_last_error(const BlowfishModule* module) {
    return module->lastError;
}