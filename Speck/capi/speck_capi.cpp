#define SPECK_BUILD_DLL
#include "speck_capi.h"
#include "../algo/speck.h"
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace std;

SpeckHandle speck_create() {
    SpeckKeySchedule* sched = static_cast<SpeckKeySchedule*>(malloc(sizeof(SpeckKeySchedule)));
    if (sched == nullptr) return nullptr;
    speckInitSchedule(*sched);
    return static_cast<SpeckHandle>(sched);
}

void speck_destroy(SpeckHandle handle) {
    if (handle != nullptr) free(handle);
}

int speck_set_key(SpeckHandle handle, const uint8_t* key, size_t keyLen) {
    if (handle == nullptr || key == nullptr) return 0;
    SpeckKeySchedule* sched = static_cast<SpeckKeySchedule*>(handle);
    vector<uint8_t> keyVec(key, key + keyLen);
    return speckSetKey(*sched, keyVec) ? 1 : 0;
}

static int copyVectorToMallocBuffer(const vector<uint8_t>& src, uint8_t** outData, size_t* outLen) {
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

int speck_encrypt_cbc(SpeckHandle handle, const uint8_t* data, size_t dataLen, const uint8_t* iv, uint8_t** outData, size_t* outLen) {
    if (handle == nullptr || iv == nullptr || outData == nullptr || outLen == nullptr) return 0;
    *outData = nullptr;
    *outLen = 0;
    SpeckKeySchedule* sched = static_cast<SpeckKeySchedule*>(handle);
    try {
        vector<uint8_t> plain(data, data + dataLen);
        vector<uint8_t> ivVec(iv, iv + SPECK_BLOCK_BYTES);
        vector<uint8_t> cipher = speckEncryptCBC(*sched, plain, ivVec);
        return copyVectorToMallocBuffer(cipher, outData, outLen);
    } catch (...) {
        return 0;
    }
}

int speck_decrypt_cbc(SpeckHandle handle, const uint8_t* data, size_t dataLen, const uint8_t* iv, uint8_t** outData, size_t* outLen) {
    if (handle == nullptr || iv == nullptr || outData == nullptr || outLen == nullptr) return 0;
    *outData = nullptr;
    *outLen = 0;
    SpeckKeySchedule* sched = static_cast<SpeckKeySchedule*>(handle);
    try {
        vector<uint8_t> cipher(data, data + dataLen);
        vector<uint8_t> ivVec(iv, iv + SPECK_BLOCK_BYTES);
        vector<uint8_t> plain = speckDecryptCBC(*sched, cipher, ivVec);
        return copyVectorToMallocBuffer(plain, outData, outLen);
    } catch (...) {
        return 0;
    }
}

void speck_free_buffer(uint8_t* buffer) {
    if (buffer != nullptr) free(buffer);
}