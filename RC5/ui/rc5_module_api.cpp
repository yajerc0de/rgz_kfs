#include "rc5_module_api.h"

using namespace std;



bool Rc5Module::load(const string& libPath) {
    if (!m_lib.load(libPath)) {
        m_lastError = "Не удалось загрузить библиотеку: " + m_lib.lastError();
        return false;
    }

    bool ok = true;
    ok = ok && bindSymbol("rc5_create",       create);
    ok = ok && bindSymbol("rc5_destroy",      destroy);
    ok = ok && bindSymbol("rc5_set_key",      setKey);
    ok = ok && bindSymbol("rc5_encrypt_cbc",  encryptCbc);
    ok = ok && bindSymbol("rc5_decrypt_cbc",  decryptCbc);
    ok = ok && bindSymbol("rc5_free_buffer",  freeBuffer);

    if (!ok) {
        m_lib.unload();
        return false;
    }

    return true;
}

bool Rc5Module::isReady() const {
    return create != nullptr && destroy != nullptr && setKey != nullptr
        && encryptCbc != nullptr && decryptCbc != nullptr && freeBuffer != nullptr;
}

const string& Rc5Module::lastError() const {
    return m_lastError;
}
