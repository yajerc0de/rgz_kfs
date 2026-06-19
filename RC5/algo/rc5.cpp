#include "rc5.h"

#include <stdexcept>
#include <cstring>
#include <algorithm>

using namespace std;


static uint32_t rc5Rotl(uint32_t x, uint32_t s) {
    s &= (RC5_W - 1);
    if (s == 0) return x;
    return (x << s) | (x >> (RC5_W - s));
}


static uint32_t rc5Rotr(uint32_t x, uint32_t s) {
    s &= (RC5_W - 1);
    if (s == 0) return x;
    return (x >> s) | (x << (RC5_W - s));
}


static void rc5PackBlock(uint32_t A, uint32_t B, uint8_t* out) {
    out[0] = (A      ) & 0xFF; out[1] = (A >>  8) & 0xFF;
    out[2] = (A >> 16) & 0xFF; out[3] = (A >> 24) & 0xFF;
    out[4] = (B      ) & 0xFF; out[5] = (B >>  8) & 0xFF;
    out[6] = (B >> 16) & 0xFF; out[7] = (B >> 24) & 0xFF;
}


static void rc5UnpackBlock(const uint8_t* in, uint32_t& A, uint32_t& B) {
    A =  uint32_t(in[0])
      | (uint32_t(in[1]) <<  8)
      | (uint32_t(in[2]) << 16)
      | (uint32_t(in[3]) << 24);
    B =  uint32_t(in[4])
      | (uint32_t(in[5]) <<  8)
      | (uint32_t(in[6]) << 16)
      | (uint32_t(in[7]) << 24);
}


static vector<uint8_t> rc5Pkcs7Pad(const vector<uint8_t>& data) {
    uint8_t padLen = static_cast<uint8_t>(RC5_BLOCK_LEN - (data.size() % RC5_BLOCK_LEN));
    vector<uint8_t> padded(data);
    padded.insert(padded.end(), padLen, padLen);
    return padded;
}


static vector<uint8_t> rc5Pkcs7Unpad(const vector<uint8_t>& data) {
    if (data.empty() || data.size() % RC5_BLOCK_LEN != 0)
        throw runtime_error("RC5: неверный размер данных при снятии паддинга");

    uint8_t padLen = data.back();
    if (padLen == 0 || padLen > RC5_BLOCK_LEN)
        throw runtime_error("RC5: повреждён PKCS#7 паддинг");

    for (size_t i = data.size() - padLen; i < data.size(); i++) {
        if (data[i] != padLen)
            throw runtime_error("RC5: неверный PKCS#7 паддинг (возможно, неверный ключ)");
    }
    return vector<uint8_t>(data.begin(), data.end() - padLen);
}


static void rc5ExpandKey(Rc5KeySchedule& sched, const vector<uint8_t>& key) {
    const int b = static_cast<int>(key.size()); 
    const int u = RC5_W / 8;                   
    const int c = (b + u - 1) / u;              

    
    vector<uint32_t> L(c, 0);
    for (int i = b - 1; i >= 0; i--)
        L[i / u] = (L[i / u] << 8) + key[i];

    
    sched.S[0] = RC5_P32;
    for (int i = 1; i < RC5_TABLE_SIZE; i++)
        sched.S[i] = sched.S[i - 1] + RC5_Q32;

    
    uint32_t A = 0, B = 0;
    int i = 0, j = 0;
    int iterations = 3 * max(RC5_TABLE_SIZE, c);

    for (int k = 0; k < iterations; k++) {
        sched.S[i] = rc5Rotl(sched.S[i] + A + B, 3);
        A = sched.S[i];
        i = (i + 1) % RC5_TABLE_SIZE;

        L[j] = rc5Rotl(L[j] + A + B, (A + B) & (RC5_W - 1));
        B = L[j];
        j = (j + 1) % c;
    }
}



void rc5InitSchedule(Rc5KeySchedule& sched) {
    memset(sched.S, 0, sizeof(sched.S));
    sched.keyIsSet = false;
}

bool rc5SetKey(Rc5KeySchedule& sched, const vector<uint8_t>& key) {
    if (key.empty() || key.size() > 255)
        return false;

    rc5ExpandKey(sched, key);
    sched.keyIsSet = true;
    return true;
}



void rc5EncryptBlock(const Rc5KeySchedule& sched, uint32_t& A, uint32_t& B) {
    A = A + sched.S[0];
    B = B + sched.S[1];

    for (int i = 1; i <= RC5_ROUNDS; i++) {
        A = rc5Rotl(A ^ B, B) + sched.S[2 * i];
        B = rc5Rotl(B ^ A, A) + sched.S[2 * i + 1];
    }
}

void rc5DecryptBlock(const Rc5KeySchedule& sched, uint32_t& A, uint32_t& B) {
    for (int i = RC5_ROUNDS; i >= 1; i--) {
        B = rc5Rotr(B - sched.S[2 * i + 1], A) ^ A;
        A = rc5Rotr(A - sched.S[2 * i],     B) ^ B;
    }

    B = B - sched.S[1];
    A = A - sched.S[0];
}



vector<uint8_t> rc5EncryptCBC(const Rc5KeySchedule& sched,
                               const vector<uint8_t>& plaintext,
                               const vector<uint8_t>& iv) {
    if (!sched.keyIsSet)
        throw runtime_error("RC5: ключ не установлен");
    if (iv.size() != RC5_BLOCK_LEN)
        throw runtime_error("RC5: IV должен быть ровно 8 байт");

    vector<uint8_t> padded = rc5Pkcs7Pad(plaintext);
    vector<uint8_t> cipher(padded.size());

    uint8_t prev[RC5_BLOCK_LEN];
    memcpy(prev, iv.data(), RC5_BLOCK_LEN);

    for (size_t offset = 0; offset < padded.size(); offset += RC5_BLOCK_LEN) {
        
        uint8_t block[RC5_BLOCK_LEN];
        for (int b = 0; b < RC5_BLOCK_LEN; b++)
            block[b] = padded[offset + b] ^ prev[b];

        uint32_t A, B;
        rc5UnpackBlock(block, A, B);
        rc5EncryptBlock(sched, A, B);
        rc5PackBlock(A, B, &cipher[offset]);

        memcpy(prev, &cipher[offset], RC5_BLOCK_LEN);
    }
    return cipher;
}

vector<uint8_t> rc5DecryptCBC(const Rc5KeySchedule& sched,
                               const vector<uint8_t>& ciphertext,
                               const vector<uint8_t>& iv) {
    if (!sched.keyIsSet)
        throw runtime_error("RC5: ключ не установлен");
    if (iv.size() != RC5_BLOCK_LEN)
        throw runtime_error("RC5: IV должен быть ровно 8 байт");
    if (ciphertext.empty() || ciphertext.size() % RC5_BLOCK_LEN != 0)
        throw runtime_error("RC5: размер шифротекста не кратен 8 байтам");

    vector<uint8_t> padded(ciphertext.size());

    uint8_t prev[RC5_BLOCK_LEN];
    memcpy(prev, iv.data(), RC5_BLOCK_LEN);

    for (size_t offset = 0; offset < ciphertext.size(); offset += RC5_BLOCK_LEN) {
        uint32_t A, B;
        rc5UnpackBlock(&ciphertext[offset], A, B);
        rc5DecryptBlock(sched, A, B);

        uint8_t block[RC5_BLOCK_LEN];
        rc5PackBlock(A, B, block);

        for (int b = 0; b < RC5_BLOCK_LEN; b++)
            padded[offset + b] = block[b] ^ prev[b];

        memcpy(prev, &ciphertext[offset], RC5_BLOCK_LEN);
    }
    return rc5Pkcs7Unpad(padded);
}
