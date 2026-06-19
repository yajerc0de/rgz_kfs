#include "serpent_module_api.h"

bool serpent_load(SerpentModule* module, const std::string& path) {
    if (module == nullptr)
        return false;

    if (!module->library.load(path)) {
        module->lastError = module->library.lastError();
        return false;
    }

    bool ok = true;

    ok = ok && serpent_bind_symbol(module, "serpent_create",      module->create);
    ok = ok && serpent_bind_symbol(module, "serpent_destroy",     module->destroy);
    ok = ok && serpent_bind_symbol(module, "serpent_set_key",     module->setKey);
    ok = ok && serpent_bind_symbol(module, "serpent_encrypt_cbc", module->encryptCbc);
    ok = ok && serpent_bind_symbol(module, "serpent_decrypt_cbc", module->decryptCbc);
    ok = ok && serpent_bind_symbol(module, "serpent_free_buffer", module->freeBuffer);

    if (!ok) {
        module->library.unload();
        return false;
    }

    return true;
}

bool serpent_is_ready(const SerpentModule* module) {
    if (module == nullptr)
        return false;

    return
        module->create     != nullptr &&
        module->destroy    != nullptr &&
        module->setKey     != nullptr &&
        module->encryptCbc != nullptr &&
        module->decryptCbc != nullptr &&
        module->freeBuffer != nullptr;
}

const std::string& serpent_last_error(const SerpentModule* module) {
    return module->lastError;
}
