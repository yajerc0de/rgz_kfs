#pragma once

#include "loader.h"
#include "../capi/rc5_capi.h"



typedef Rc5Handle (*Rc5CreateFn)();
typedef void      (*Rc5DestroyFn)(Rc5Handle);
typedef int       (*Rc5SetKeyFn)(Rc5Handle, const uint8_t*, size_t);
typedef int       (*Rc5EncryptCbcFn)(Rc5Handle, const uint8_t*, size_t,
                                      const uint8_t*, uint8_t**, size_t*);
typedef int       (*Rc5DecryptCbcFn)(Rc5Handle, const uint8_t*, size_t,
                                      const uint8_t*, uint8_t**, size_t*);
typedef void      (*Rc5FreeBufferFn)(uint8_t*);



struct Rc5Module {
    DynamicLibrary lib;
    string         lastError;

    Rc5CreateFn      create     = nullptr;
    Rc5DestroyFn     destroy    = nullptr;
    Rc5SetKeyFn      setKey     = nullptr;
    Rc5EncryptCbcFn  encryptCbc = nullptr;
    Rc5DecryptCbcFn  decryptCbc = nullptr;
    Rc5FreeBufferFn  freeBuffer = nullptr;
};


bool rc5ModuleLoad(Rc5Module& mod, const string& libPath);


bool rc5ModuleIsReady(const Rc5Module& mod);
