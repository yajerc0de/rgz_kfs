// =============================================================================
//  blowfish_capi.cpp — реализация C-интерфейса для .dll/.so
//
//  BLOWFISH_BUILD_DLL определён только при компиляции этой библиотеки
//  (через CMakeLists.txt) — это активирует __declspec(dllexport)
//  в blowfish_capi.h (см. условие там).
// =============================================================================

#include "blowfish_capi.h"
#include "../algo/blowfish.h"

#include <cstdlib>
#include <cstring>
#include <vector>

using namespace std;

// ── Жизненный цикл ────────────────────────────────────────────────────────────

BlowfishHandle blowfish_create() {
    try {
        return static_cast<BlowfishHandle>(new Blowfish());
    } catch (...) {
        return nullptr;
    }
}

void blowfish_destroy(BlowfishHandle handle) {
    if (handle == nullptr) return;
    delete static_cast<Blowfish*>(handle);
}

int blowfish_set_key(BlowfishHandle handle, const uint8_t* key, size_t keyLen) {
    if (handle == nullptr || key == nullptr) return 0;

    Blowfish* bf = static_cast<Blowfish*>(handle);
    vector<uint8_t> keyVec(key, key + keyLen);

    return bf->setKey(keyVec) ? 1 : 0;
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

int blowfish_encrypt_cbc(BlowfishHandle handle,
                          const uint8_t* data, size_t dataLen,
                          const uint8_t* iv,
                          uint8_t** outData, size_t* outLen)
{
    if (handle == nullptr || iv == nullptr || outData == nullptr || outLen == nullptr)
        return 0;

    *outData = nullptr;
    *outLen  = 0;

    Blowfish* bf = static_cast<Blowfish*>(handle);

    try {
        vector<uint8_t> plain(data, data + dataLen);
        vector<uint8_t> ivVec(iv, iv + BLOWFISH_BLOCK_BYTES);

        vector<uint8_t> cipher = bf->encryptCBC(plain, ivVec);

        return copyVectorToMallocBuffer(cipher, outData, outLen);
    } catch (...) {
        return 0;
    }
}

int blowfish_decrypt_cbc(BlowfishHandle handle,
                          const uint8_t* data, size_t dataLen,
                          const uint8_t* iv,
                          uint8_t** outData, size_t* outLen)
{
    if (handle == nullptr || iv == nullptr || outData == nullptr || outLen == nullptr)
        return 0;

    *outData = nullptr;
    *outLen  = 0;

    Blowfish* bf = static_cast<Blowfish*>(handle);

    try {
        vector<uint8_t> cipher(data, data + dataLen);
        vector<uint8_t> ivVec(iv, iv + BLOWFISH_BLOCK_BYTES);

        vector<uint8_t> plain = bf->decryptCBC(cipher, ivVec);

        return copyVectorToMallocBuffer(plain, outData, outLen);
    } catch (...) {
        // Сюда попадаем при неверном ключе, повреждённом паддинге и т.п.
        return 0;
    }
}

void blowfish_free_buffer(uint8_t* buffer) {
    if (buffer != nullptr) {
        free(buffer);
    }
}