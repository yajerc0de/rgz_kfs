#pragma once

#include "../../../core/loader.h"
#include "../capi/speck_capi.h"
#include <string>

typedef SpeckHandle (*SpeckCreateFn)();
typedef void        (*SpeckDestroyFn)(SpeckHandle);
typedef int         (*SpeckSetKeyFn)(SpeckHandle, const uint8_t*, size_t);
typedef int         (*SpeckEncryptCbcFn)(SpeckHandle, const uint8_t*, size_t, const uint8_t*, uint8_t**, size_t*);
typedef int         (*SpeckDecryptCbcFn)(SpeckHandle, const uint8_t*, size_t, const uint8_t*, uint8_t**, size_t*);
typedef void        (*SpeckFreeBufferFn)(uint8_t*);

struct SpeckModule {
    DynamicLibrary lib;
    std::string lastError;
    
    SpeckCreateFn      create     = nullptr;
    SpeckDestroyFn     destroy    = nullptr;
    SpeckSetKeyFn      setKey     = nullptr;
    SpeckEncryptCbcFn  encryptCbc = nullptr;
    SpeckDecryptCbcFn  decryptCbc = nullptr;
    SpeckFreeBufferFn  freeBuffer = nullptr;
};

bool speckModuleLoad(SpeckModule& mod, const std::string& libPath);
bool speckModuleIsReady(const SpeckModule& mod);