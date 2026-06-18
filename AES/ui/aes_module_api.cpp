#include "aes_module_api.h"

bool aes_load(AESModule* module, const std::string& path) {
    if (module == nullptr)
    return false;

    if (!module->library.load(path)){
        module->lastError = module->library.lastError();

        return false;
    }

    bool ok = true;

    ok = ok && aes_bind_symbol(module, "aes_create", module->create);

    ok = ok && aes_bind_symbol(module, "aes_destroy", module->destroy);

    ok = ok && aes_bind_symbol(module, "aes_set_key", module->setKey);

    ok = ok && aes_bind_symbol(module, "aes_encrypt_cbc", module->encryptCbc);

    ok = ok && aes_bind_symbol(module, "aes_decrypt_cbc", module->decryptCbc);

    ok = ok && aes_bind_symbol(module, "aes_free_buffer", module->freeBuffer);

    if (!ok){
        module->library.unload();
        return false;
    }

    return true;

}

bool aes_is_ready(const AESModule* module) {
    if (module == nullptr)
    return false;

    return
        module->create != nullptr &&
        module->destroy != nullptr &&
        module->setKey != nullptr &&
        module->encryptCbc != nullptr &&
        module->decryptCbc != nullptr &&
        module->freeBuffer != nullptr;
}

const std::string& aes_last_error(const AESModule* module) {
    return module->lastError;
}
