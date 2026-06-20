#pragma once

#include <cstdint>
#include <cstddef>



#ifdef _WIN32
    #ifdef RC5_BUILD_DLL
        #define RC5_API __declspec(dllexport)
    #else
        #define RC5_API __declspec(dllimport)
    #endif
#else
    #define RC5_API __attribute__((visibility("default")))
#endif

extern "C" {

typedef void* Rc5Handle;

#define RC5_BLOCK_BYTES 8
#define RC5_KEY_BYTES   16


RC5_API Rc5Handle rc5_create();


RC5_API void rc5_destroy(Rc5Handle handle);


RC5_API int rc5_set_key(Rc5Handle handle, const uint8_t* key, size_t keyLen);



RC5_API int rc5_encrypt_cbc(Rc5Handle handle,
                             const uint8_t* data, size_t dataLen,
                             const uint8_t* iv,
                             uint8_t** outData, size_t* outLen);

RC5_API int rc5_decrypt_cbc(Rc5Handle handle,
                             const uint8_t* data, size_t dataLen,
                             const uint8_t* iv,
                             uint8_t** outData, size_t* outLen);


RC5_API void rc5_free_buffer(uint8_t* buffer);

} // extern "C"