#pragma once

#include "../../core/loader.h"
#include "../capi/serpent_capi.h"

#include <string>


typedef SerpentHandle (*SerpentCreateFn)();

typedef void (*SerpentDestroyFn)(SerpentHandle);

typedef int (*SerpentSetKeyFn)(SerpentHandle, const uint8_t*, size_t);

typedef int (*SerpentEncryptCbcFn)(SerpentHandle, const uint8_t*, size_t, const uint8_t*, uint8_t**, size_t*);

typedef int (*SerpentDecryptCbcFn)(SerpentHandle, const uint8_t*, size_t, const uint8_t*, uint8_t**, size_t*);

typedef void (*SerpentFreeBufferFn)(uint8_t*);

struct SerpentModule {
    DynamicLibrary library;

    SerpentCreateFn create = nullptr;
    SerpentDestroyFn destroy = nullptr;
    SerpentSetKeyFn setKey = nullptr;

    SerpentEncryptCbcFn encryptCbc = nullptr;
    SerpentDecryptCbcFn decryptCbc = nullptr;

    SerpentFreeBufferFn freeBuffer = nullptr;

    std::string lastError;
};

bool serpent_load(SerpentModule* module, const std::string& path);

bool serpent_is_ready(const SerpentModule* module);

const std::string& serpent_last_error(const SerpentModule* module);

template<typename FnPtr>
bool serpent_bind_symbol(SerpentModule* module, const std::string& name, FnPtr& target) {
    void* sym = module->library.getSymbol(name);

    if (sym == nullptr)
    {
        module->lastError =
            "Не найден символ: " + name;

        return false;
    }

    target = reinterpret_cast<FnPtr>(sym);

    return true;
}
