// =============================================================================
//  rc5_capi.cpp — реализация C-интерфейса для .dll/.so
// =============================================================================

#define RC5_BUILD_DLL
#include "rc5_capi.h"
#include "../algo/rc5.h"

#include <cstdlib>
#include <cstring>
#include <vector>
#include <stdexcept>

using namespace std;

// ── Жизненный цикл ────────────────────────────────────────────────────────────

Rc5Handle rc5_create() {
    Rc5KeySchedule* sched = static_cast<Rc5KeySchedule*>(malloc(sizeof(Rc5KeySchedule)));
    if (sched == nullptr) return nullptr;

    rc5InitSchedule(*sched);
    return static_cast<Rc5Handle>(sched);
}

void rc5_destroy(Rc5Handle handle) {
    if (handle == nullptr) return;
    free(handle);
}

int rc5_set_key(Rc5Handle handle, const uint8_t* key, size_t keyLen) {
    if (handle == nullptr || key == nullptr) return 0;

    Rc5KeySchedule* sched = static_cast<Rc5KeySchedule*>(handle);
    vector<uint8_t> keyVec(key, key + keyLen);

    return rc5SetKey(*sched, keyVec) ? 1 : 0;
}

// ── Вспомогательная функция: скопировать vector<uint8_t> в malloc-буфер ───────

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

// ── CBC-шифрование / дешифрование ───────────────────────────────────────────

int rc5_encrypt_cbc(Rc5Handle handle,
                     const uint8_t* data, size_t dataLen,
                     const uint8_t* iv,
                     uint8_t** outData, size_t* outLen)
{
    if (handle == nullptr || iv == nullptr || outData == nullptr || outLen == nullptr)
        return 0;

    *outData = nullptr;
    *outLen  = 0;

    Rc5KeySchedule* sched = static_cast<Rc5KeySchedule*>(handle);

    try {
        vector<uint8_t> plain(data, data + dataLen);
        vector<uint8_t> ivVec(iv, iv + RC5_BLOCK_BYTES);

        vector<uint8_t> cipher = rc5EncryptCBC(*sched, plain, ivVec);

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

    Rc5KeySchedule* sched = static_cast<Rc5KeySchedule*>(handle);

    try {
        vector<uint8_t> cipher(data, data + dataLen);
        vector<uint8_t> ivVec(iv, iv + RC5_BLOCK_BYTES);

        vector<uint8_t> plain = rc5DecryptCBC(*sched, cipher, ivVec);

        return copyVectorToMallocBuffer(plain, outData, outLen);
    } catch (...) {
        // Сюда попадаем при неверном ключе, повреждённом паддинге и т.п.
        return 0;
    }
}

void rc5_free_buffer(uint8_t* buffer) {
    if (buffer != nullptr) {
        free(buffer);
    }
}
