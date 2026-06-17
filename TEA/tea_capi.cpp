// =============================================================================
//  tea_capi.cpp — реализация C-интерфейса для .dll/.so
//
//  TEA_BUILD_DLL определён только при компиляции этой библиотеки —
//  это активирует __declspec(dllexport) в tea_capi.h (см. условие там).
// =============================================================================

#define TEA_BUILD_DLL
#include "tea_capi.h"
#include "tea.h"

#include <cstdlib>
#include <cstring>
#include <vector>
#include <stdexcept>

using namespace std;

// ── Жизненный цикл ────────────────────────────────────────────────────────────

TeaHandle tea_create() {
    try {
        return static_cast<TeaHandle>(new TEA());
    } catch (...) {
        return nullptr;
    }
}

void tea_destroy(TeaHandle handle) {
    if (handle == nullptr) return;
    delete static_cast<TEA*>(handle);
}

int tea_set_key(TeaHandle handle, const uint8_t* key, size_t keyLen) {
    if (handle == nullptr || key == nullptr) return 0;

    TEA* tea = static_cast<TEA*>(handle);
    vector<uint8_t> keyVec(key, key + keyLen);

    return tea->setKey(keyVec) ? 1 : 0;
}

// ── Вспомогательная функция: скопировать vector<uint8_t> в malloc-буфер ───────
// Используется и для encrypt, и для decrypt, чтобы не дублировать код.

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

int tea_encrypt_cbc(TeaHandle handle,
                     const uint8_t* data, size_t dataLen,
                     const uint8_t* iv,
                     uint8_t** outData, size_t* outLen)
{
    if (handle == nullptr || iv == nullptr || outData == nullptr || outLen == nullptr)
        return 0;

    *outData = nullptr;
    *outLen  = 0;

    TEA* tea = static_cast<TEA*>(handle);

    try {
        vector<uint8_t> plain(data, data + dataLen);
        vector<uint8_t> ivVec(iv, iv + TEA_BLOCK_BYTES);

        vector<uint8_t> cipher = tea->encryptCBC(plain, ivVec);

        return copyVectorToMallocBuffer(cipher, outData, outLen);
    } catch (...) {
        return 0;
    }
}

int tea_decrypt_cbc(TeaHandle handle,
                     const uint8_t* data, size_t dataLen,
                     const uint8_t* iv,
                     uint8_t** outData, size_t* outLen)
{
    if (handle == nullptr || iv == nullptr || outData == nullptr || outLen == nullptr)
        return 0;

    *outData = nullptr;
    *outLen  = 0;

    TEA* tea = static_cast<TEA*>(handle);

    try {
        vector<uint8_t> cipher(data, data + dataLen);
        vector<uint8_t> ivVec(iv, iv + TEA_BLOCK_BYTES);

        vector<uint8_t> plain = tea->decryptCBC(cipher, ivVec);

        return copyVectorToMallocBuffer(plain, outData, outLen);
    } catch (...) {
        // Сюда попадаем при неверном ключе, повреждённом паддинге и т.п.
        return 0;
    }
}

void tea_free_buffer(uint8_t* buffer) {
    if (buffer != nullptr) {
        free(buffer);
    }
}