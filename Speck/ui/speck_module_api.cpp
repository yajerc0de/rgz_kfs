#include "speck_module_api.h"

using namespace std;

template <typename FnPtr>
static bool speckBindSymbol(SpeckModule& mod, const string& name, FnPtr& target) {
    void* sym = mod.lib.getSymbol(name);
    if (sym == nullptr) {
        mod.lastError = "Error: " + name;
        return false;
    }
    target = reinterpret_cast<FnPtr>(sym);
    return true;
}

bool speckModuleLoad(SpeckModule& mod, const string& libPath) {
    if (!mod.lib.load(libPath)) {
        mod.lastError = mod.lib.lastError();
        return false;
    }

    bool ok = true;
    ok = ok && speckBindSymbol(mod, "speck_create",      mod.create);
    ok = ok && speckBindSymbol(mod, "speck_destroy",     mod.destroy);
    ok = ok && speckBindSymbol(mod, "speck_set_key",     mod.setKey);
    ok = ok && speckBindSymbol(mod, "speck_encrypt_cbc", mod.encryptCbc);
    ok = ok && speckBindSymbol(mod, "speck_decrypt_cbc", mod.decryptCbc);
    ok = ok && speckBindSymbol(mod, "speck_free_buffer", mod.freeBuffer);

    if (!ok) {
        mod.lib.unload();
        return false;
    }
    return true;
}

bool speckModuleIsReady(const SpeckModule& mod) {
    return mod.create && mod.destroy && mod.setKey && mod.encryptCbc && mod.decryptCbc && mod.freeBuffer;
}