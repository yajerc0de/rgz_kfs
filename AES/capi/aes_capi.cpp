#include "aes_capi.h"

#include "../algo/aes_core.h"
#include "../algo/aes_cbc.h"

#include <vector>
#include <cstdlib>
#include <cstring>

using namespace std;

struct AESContext
{
unsigned char roundKeys[ROUND_KEYS_SIZE];
bool keyReady;
};

static int copyVectorToMallocBuffer( const vector<unsigned char>& src, uint8_t** outData, size_t* outLen) {
    uint8_t* buffer = static_cast<uint8_t*>(malloc(src.size()));

    if (buffer == nullptr) {
        *outData = nullptr;
        *outLen = 0;
        return 0;
    }

    memcpy(buffer, src.data(),src.size());

    *outData = buffer;
    *outLen = src.size();

    return 1;
}

AESHandle aes_create(){
    AESContext* ctx = new AESContext;

    ctx->keyReady = false;

    memset(ctx->roundKeys, 0, sizeof(ctx->roundKeys));

    return ctx;
}

void aes_destroy(AESHandle handle) {
    if (handle == nullptr) return;

    delete static_cast<AESContext*>(handle);
}

int aes_set_key(AESHandle handle, const uint8_t* key, size_t keyLen) {
    if (handle == nullptr) return 0;

    if (key == nullptr) return 0;

    if (keyLen != AES_KEY_BYTES) return 0;

    AESContext* ctx = static_cast<AESContext*>(handle);

    expand_key(key, ctx->roundKeys);

    ctx->keyReady = true;

    return 1;
}

int aes_encrypt_cbc(AESHandle handle, const uint8_t* data, size_t dataLen, const uint8_t* iv, uint8_t** outData, size_t* outLen) {
    if (handle == nullptr) return 0;

    if (data == nullptr) return 0;

    if (iv == nullptr) return 0;

    if (outData == nullptr) return 0;

    if (outLen == nullptr) return 0;

    AESContext* ctx = static_cast<AESContext*>(handle);

    if (!ctx->keyReady) return 0;

    *outData = nullptr;
    *outLen = 0;

    vector<unsigned char> plain(data,data + dataLen);

    vector<unsigned char> cipher = cbc_encrypt(plain, iv, ctx->roundKeys);

    return copyVectorToMallocBuffer(cipher, outData, outLen);

}

int aes_decrypt_cbc(AESHandle handle, const uint8_t* data, size_t dataLen, const uint8_t* iv, uint8_t** outData, size_t* outLen) {
    if (handle == nullptr) return 0;

    if (data == nullptr) return 0;

    if (iv == nullptr) return 0;

    if (outData == nullptr) return 0;

    if (outLen == nullptr) return 0;

    AESContext* ctx = static_cast<AESContext*>(handle);

    if (!ctx->keyReady) return 0;

    *outData = nullptr;
    *outLen = 0;

    vector<unsigned char> cipher(data, data + dataLen);

    vector<unsigned char> plain = cbc_decrypt(cipher, iv, ctx->roundKeys);

    return copyVectorToMallocBuffer(plain, outData, outLen);
}

void aes_free_buffer(uint8_t* buffer) {
    if (buffer != nullptr) {
        free(buffer);
    }
}