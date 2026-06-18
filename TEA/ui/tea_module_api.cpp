#include "tea_module_api.h"

using namespace std;

// =============================================================================
//  load — загрузить библиотеку и связать все функции
// =============================================================================

bool TeaModule::load(const string& libPath) {
    if (!m_lib.load(libPath)) {
        m_lastError = "Не удалось загрузить библиотеку: " + m_lib.lastError();
        return false;
    }

    // Привязываем каждый символ. Если хотя бы один не найден — библиотека
    // считается несовместимой (например собрана от другой версии tea_capi.h).
    bool ok = true;
    ok = ok && bindSymbol("tea_create",       create);
    ok = ok && bindSymbol("tea_destroy",      destroy);
    ok = ok && bindSymbol("tea_set_key",      setKey);
    ok = ok && bindSymbol("tea_encrypt_cbc",  encryptCbc);
    ok = ok && bindSymbol("tea_decrypt_cbc",  decryptCbc);
    ok = ok && bindSymbol("tea_free_buffer",  freeBuffer);

    if (!ok) {
        m_lib.unload();
        return false;
    }

    return true;
}

bool TeaModule::isReady() const {
    return create != nullptr && destroy != nullptr && setKey != nullptr
        && encryptCbc != nullptr && decryptCbc != nullptr && freeBuffer != nullptr;
}

const string& TeaModule::lastError() const {
    return m_lastError;
}