#pragma once

#include "../../core/loader.h"
#include "../capi/rc5_capi.h"



typedef Rc5Handle (*Rc5CreateFn)();
typedef void      (*Rc5DestroyFn)(Rc5Handle);
typedef int       (*Rc5SetKeyFn)(Rc5Handle, const uint8_t*, size_t);
typedef int       (*Rc5EncryptCbcFn)(Rc5Handle, const uint8_t*, size_t,
                                      const uint8_t*, uint8_t**, size_t*);
typedef int       (*Rc5DecryptCbcFn)(Rc5Handle, const uint8_t*, size_t,
                                      const uint8_t*, uint8_t**, size_t*);
typedef void      (*Rc5FreeBufferFn)(uint8_t*);

class Rc5Module {
public:

    bool load(const string& libPath);

    
    bool isReady() const;

   
    const string& lastError() const;

    
    Rc5CreateFn      create      = nullptr;
    Rc5DestroyFn     destroy     = nullptr;
    Rc5SetKeyFn      setKey      = nullptr;
    Rc5EncryptCbcFn  encryptCbc  = nullptr;
    Rc5DecryptCbcFn  decryptCbc  = nullptr;
    Rc5FreeBufferFn  freeBuffer  = nullptr;

private:
    DynamicLibrary m_lib;
    string         m_lastError;

    
    template <typename FnPtr>
    bool bindSymbol(const string& name, FnPtr& target);
};



template <typename FnPtr>
bool Rc5Module::bindSymbol(const string& name, FnPtr& target) {
    void* sym = m_lib.getSymbol(name);
    if (sym == nullptr) {
        m_lastError = "Не найден символ в библиотеке: " + name;
        return false;
    }
    target = reinterpret_cast<FnPtr>(sym);
    return true;
}
