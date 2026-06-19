#pragma once

#include <cstdint>
#include <cstddef>
#include "../algo/aes_export.h"

using namespace std;

extern "C"
{
typedef void* AESHandle;

#define AES_BLOCK_BYTES 16
#define AES_KEY_BYTES   16

AES_API AESHandle aes_create();

AES_API void aes_destroy(AESHandle handle);

AES_API int aes_set_key(AESHandle handle, const uint8_t* key, size_t keyLen);

AES_API int aes_encrypt_cbc(AESHandle handle, const uint8_t* data, size_t dataLen, const uint8_t* iv, uint8_t** outData, size_t* outLen);

AES_API int aes_decrypt_cbc(AESHandle handle, const uint8_t* data, size_t dataLen, const uint8_t* iv, uint8_t** outData, size_t* outLen);

AES_API void aes_free_buffer(uint8_t* buffer);

}