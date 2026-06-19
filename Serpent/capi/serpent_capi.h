#pragma once

#include <cstdint>
#include <cstddef>

#ifdef _WIN32
#ifdef SERPENT_BUILD_DLL
#define SERPENT_API __declspec(dllexport)
#else
#define SERPENT_API __declspec(dllimport)
#endif
#else
#define SERPENT_API __attribute__((visibility("default")))
#endif

extern "C"
{
typedef void* SerpentHandle;

#define SERPENT_BLOCK_BYTES 16
#define SERPENT_KEY_BYTES   16

SERPENT_API SerpentHandle serpent_create();

SERPENT_API void serpent_destroy(SerpentHandle handle);

SERPENT_API int serpent_set_key(SerpentHandle handle, const uint8_t* key, size_t keyLen);

SERPENT_API int serpent_encrypt_cbc(SerpentHandle handle, const uint8_t* data, size_t dataLen, const uint8_t* iv, uint8_t** outData, size_t* outLen);

SERPENT_API int serpent_decrypt_cbc(SerpentHandle handle, const uint8_t* data, size_t dataLen, const uint8_t* iv, uint8_t** outData, size_t* outLen);

SERPENT_API void serpent_free_buffer(uint8_t* buffer);

}
