#pragma once

#include "../../core/loader.h"
#include "../capi/aes_capi.h"

#include <string>


typedef AESHandle (*AESCreateFn)();

typedef void (*AESDestroyFn)(AESHandle);

typedef int (*AESSetKeyFn)(AESHandle, const uint8_t*, size_t);

typedef int (*AESEncryptCbcFn)(AESHandle, const uint8_t*, size_t, const uint8_t*, uint8_t**, size_t*);

typedef int (*AESDecryptCbcFn)(AESHandle, const uint8_t*, size_t, const uint8_t*, uint8_t**, size_t*);

typedef void (*AESFreeBufferFn)(uint8_t*);

struct AESModule {
    DynamicLibrary library;

    AESCreateFn create = nullptr;
    AESDestroyFn destroy = nullptr;
    AESSetKeyFn setKey = nullptr;

    AESEncryptCbcFn encryptCbc = nullptr;
    AESDecryptCbcFn decryptCbc = nullptr;

    AESFreeBufferFn freeBuffer = nullptr;

    std::string lastError;

};

bool aes_load(AESModule* module, const std::string& path);

bool aes_is_ready(const AESModule* module);

const std::string& aes_last_error(const AESModule* module);

template<typename FnPtr>
bool aes_bind_symbol(AESModule* module,const std::string& name,FnPtr& target){
    void* sym = module->library.getSymbol(name);

    if (sym == nullptr)
    {
        module->lastError =
            "Не найден символ: " + name;

        return false;
    }

    target =reinterpret_cast<FnPtr>(sym);

    return true;

}
