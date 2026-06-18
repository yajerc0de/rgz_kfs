#include "blowfish_module_api.h"

using namespace std;

// =============================================================================
//  load — загрузить библиотеку и связать все функции
// =============================================================================

bool BlowfishModule::load(const string& libPath) {
    if (!m_lib.load(libPath)) {
        m_lastError = "Не удалось загрузить библиотеку: " + m_lib.lastError();
        return false;
    }

    // Привязываем каждый символ. Если хотя бы один не найден — библиотека
    // считается несовместимой (например собрана от другой версии capi-заголовка).
    bool ok = true;
    ok = ok && bindSymbol("blowfish_create",       create);
    ok = ok && bindSymbol("blowfish_destroy",      destroy);
    ok = ok && bindSymbol("blowfish_set_key",      setKey);
    ok = ok && bindSymbol("blowfish_encrypt_cbc",  encryptCbc);
    ok = ok && bindSymbol("blowfish_decrypt_cbc",  decryptCbc);
    ok = ok && bindSymbol("blowfish_free_buffer",  freeBuffer);

    if (!ok) {
        m_lib.unload();
        return false;
    }

    return true;
}

bool BlowfishModule::isReady() const {
    return create != nullptr && destroy != nullptr && setKey != nullptr
        && encryptCbc != nullptr && decryptCbc != nullptr && freeBuffer != nullptr;
}

const string& BlowfishModule::lastError() const {
    return m_lastError;
}