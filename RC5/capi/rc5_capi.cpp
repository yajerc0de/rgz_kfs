

#define RC5_BUILD_DLL
#include "rc5_capi.h"
#include "../algo/rc5.h"

#include <cstdlib>
#include <cstring>
#include <vector>
#include <stdexcept>

using namespace std;



Rc5Handle rc5_create() {
    try {
        return static_cast<Rc5Handle>(new RC5());
    } catch (...) {
        return nullptr;
    }
}

void rc5_destroy(Rc5Handle handle) {
    if (handle == nullptr) return;
    delete static_cast<RC5*>(handle);
}

int rc5_set_key(Rc5Handle handle, const uint8_t* key, size_t keyLen) {
    if (handle == nullptr || key == nullptr) return 0;

    RC5* rc5 = static_cast<RC5*>(handle);
    vector<uint8_t> keyVec(key, key + keyLen);

    return rc5->setKey(keyVec) ? 1 : 0;
}



static int copyVectorToMallocBuffer(const vector<uint8_t>& src,
                                     uint8_t** outData, size_t* outLen)
{
    uint8_t* buffer = static_cast<uint8_t*>(malloc(src.size()));
    if (buffer == nullptr) {
        *outData = nullptr;
        *outLen  = 0;
        return 0;
    }

    memcpy(buffer, src.data(), src.size());
    *outData = buffer;
    *outLen  = src.size();
    return 1;
}



int rc5_encrypt_cbc(Rc5Handle handle,
                     const uint8_t* data, size_t dataLen,
                     const uint8_t* iv,
                     uint8_t** outData, size_t* outLen)
{
    if (handle == nullptr || iv == nullptr || outData == nullptr || outLen == nullptr)
        return 0;

    *outData = nullptr;
    *outLen  = 0;

    RC5* rc5 = static_cast<RC5*>(handle);

    try {
        vector<uint8_t> plain(data, data + dataLen);
        vector<uint8_t> ivVec(iv, iv + RC5_BLOCK_BYTES);

        vector<uint8_t> cipher = rc5->encryptCBC(plain, ivVec);

        return copyVectorToMallocBuffer(cipher, outData, outLen);
    } catch (...) {
        return 0;
    }
}

int rc5_decrypt_cbc(Rc5Handle handle,
                     const uint8_t* data, size_t dataLen,
                     const uint8_t* iv,
                     uint8_t** outData, size_t* outLen)
{
    if (handle == nullptr || iv == nullptr || outData == nullptr || outLen == nullptr)
        return 0;

    *outData = nullptr;
    *outLen  = 0;

    RC5* rc5 = static_cast<RC5*>(handle);

    try {
        vector<uint8_t> cipher(data, data + dataLen);
        vector<uint8_t> ivVec(iv, iv + RC5_BLOCK_BYTES);

        vector<uint8_t> plain = rc5->decryptCBC(cipher, ivVec);

        return copyVectorToMallocBuffer(plain, outData, outLen);
    } catch (...) {
        
        return 0;
    }
}

void rc5_free_buffer(uint8_t* buffer) {
    if (buffer != nullptr) {
        free(buffer);
    }
}
