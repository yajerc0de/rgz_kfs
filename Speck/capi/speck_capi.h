#pragma once

#include <cstdint>
#include <cstddef>

#ifdef _WIN32
    #ifdef SPECK_BUILD_DLL
        #define SPECK_API __declspec(dllexport)
    #else
        #define SPECK_API __declspec(dllimport)
    #endif
#else
    #define SPECK_API __attribute__((visibility("default")))
#endif

extern "C" {

typedef void* SpeckHandle;

#define SPECK_BLOCK_BYTES 16

SPECK_API SpeckHandle speck_create();

SPECK_API void speck_destroy(SpeckHandle handle);

SPECK_API int speck_set_key(SpeckHandle handle, const uint8_t* key, size_t keyLen);

SPECK_API int speck_encrypt_cbc(SpeckHandle handle,
                                const uint8_t* data, size_t dataLen,
                                const uint8_t* iv,
                                uint8_t** outData, size_t* outLen);

SPECK_API int speck_decrypt_cbc(SpeckHandle handle,
                                const uint8_t* data, size_t dataLen,
                                const uint8_t* iv,
                                uint8_t** outData, size_t* outLen);

SPECK_API void speck_free_buffer(uint8_t* buffer);

}