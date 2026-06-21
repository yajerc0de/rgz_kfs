#include "rc5_module_api.h"

using namespace std;



template <typename FnPtr>
static bool rc5BindSymbol(Rc5Module& mod, const string& name, FnPtr& target) {
    void* sym = mod.lib.getSymbol(name);
    if (sym == nullptr) {
        mod.lastError = "Не найден символ в библиотеке: " + name;
        return false;
    }
    target = reinterpret_cast<FnPtr>(sym);
    return true;
}



bool rc5ModuleLoad(Rc5Module& mod, const string& libPath) {
    if (!mod.lib.load(libPath)) {
        mod.lastError = "Не удалось загрузить библиотеку: " + mod.lib.lastError();
        return false;
    }

    
    bool ok = true;
    ok = ok && rc5BindSymbol(mod, "rc5_create",      mod.create);
    ok = ok && rc5BindSymbol(mod, "rc5_destroy",     mod.destroy);
    ok = ok && rc5BindSymbol(mod, "rc5_set_key",     mod.setKey);
    ok = ok && rc5BindSymbol(mod, "rc5_encrypt_cbc", mod.encryptCbc);
    ok = ok && rc5BindSymbol(mod, "rc5_decrypt_cbc", mod.decryptCbc);
    ok = ok && rc5BindSymbol(mod, "rc5_free_buffer", mod.freeBuffer);

    if (!ok) {
        mod.lib.unload();
        return false;
    }

    return true;
}

bool rc5ModuleIsReady(const Rc5Module& mod) {
    return mod.create != nullptr && mod.destroy != nullptr && mod.setKey != nullptr
        && mod.encryptCbc != nullptr && mod.decryptCbc != nullptr && mod.freeBuffer != nullptr;
}
