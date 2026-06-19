#include "serpent_capi.h"

#include "../algo/serpent_core.h"
#include "../algo/serpent_cbc.h"

#include <vector>
#include <cstdlib>
#include <cstring>

using namespace std;

struct SerpentContext
{
    unsigned int subkeys[SUBKEYS_COUNT];
    bool keyReady;
};

static int copyVectorToMallocBuffer(const vector<unsigned char>& src, uint8_t** outData, size_t* outLen) {
    uint8_t* buffer = static_cast<uint8_t*>(malloc(src.size()));

    if (buffer == nullptr) {
        *outData = nullptr;
        *outLen = 0;
        return 0;
    }

    memcpy(buffer, src.data(), src.size());

    *outData = buffer;
    *outLen = src.size();

    return 1;
}

SerpentHandle serpent_create() {
    SerpentContext* ctx = new SerpentContext;

    ctx->keyReady = false;
    memset(ctx->subkeys, 0, sizeof(ctx->subkeys));

    return ctx;
}

void serpent_destroy(SerpentHandle handle) {
    if (handle == nullptr) return;

    delete static_cast<SerpentContext*>(handle);
}

int serpent_set_key(SerpentHandle handle, const uint8_t* key, size_t keyLen) {
    if (handle == nullptr) return 0;
    if (key == nullptr) return 0;
    if (keyLen != SERPENT_KEY_BYTES) return 0;

    SerpentContext* ctx = static_cast<SerpentContext*>(handle);

    serpent_expand_key(key, ctx->subkeys);

    ctx->keyReady = true;

    return 1;
}

int serpent_encrypt_cbc(SerpentHandle handle, const uint8_t* data, size_t dataLen, const uint8_t* iv, uint8_t** outData, size_t* outLen) {
    if (handle == nullptr) return 0;
    if (data == nullptr) return 0;
    if (iv == nullptr) return 0;
    if (outData == nullptr) return 0;
    if (outLen == nullptr) return 0;

    SerpentContext* ctx = static_cast<SerpentContext*>(handle);

    if (!ctx->keyReady) return 0;

    *outData = nullptr;
    *outLen = 0;

    vector<unsigned char> plain(data, data + dataLen);
    vector<unsigned char> cipher = serpent_cbc_encrypt(plain, iv, ctx->subkeys);

    return copyVectorToMallocBuffer(cipher, outData, outLen);
}

int serpent_decrypt_cbc(SerpentHandle handle, const uint8_t* data, size_t dataLen, const uint8_t* iv, uint8_t** outData, size_t* outLen) {
    if (handle == nullptr) return 0;
    if (data == nullptr) return 0;
    if (iv == nullptr) return 0;
    if (outData == nullptr) return 0;
    if (outLen == nullptr) return 0;

    SerpentContext* ctx = static_cast<SerpentContext*>(handle);

    if (!ctx->keyReady) return 0;

    *outData = nullptr;
    *outLen = 0;

    vector<unsigned char> cipherVec(data, data + dataLen);
    vector<unsigned char> plain = serpent_cbc_decrypt(cipherVec, iv, ctx->subkeys);

    return copyVectorToMallocBuffer(plain, outData, outLen);
}

void serpent_free_buffer(uint8_t* buffer) {
    if (buffer != nullptr) {
        free(buffer);
    }
}
