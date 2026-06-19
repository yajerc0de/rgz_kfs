#pragma once

#include <cstdint>
#include <vector>
#include <string>

using namespace std;



constexpr int      RC5_WORD_BITS   = 32;
constexpr uint32_t RC5_W           = RC5_WORD_BITS;
constexpr int      RC5_ROUNDS      = 12;
constexpr int      RC5_KEY_BYTES_DEFAULT = 16;
constexpr int      RC5_BLOCK_LEN   = 8;                       
constexpr int      RC5_TABLE_SIZE  = 2 * (RC5_ROUNDS + 1);    


constexpr uint32_t RC5_P32 = 0xB7E15163u;
constexpr uint32_t RC5_Q32 = 0x9E3779B9u;



struct Rc5KeySchedule {
    uint32_t S[RC5_TABLE_SIZE];
    bool     keyIsSet;
};


void rc5InitSchedule(Rc5KeySchedule& sched);


bool rc5SetKey(Rc5KeySchedule& sched, const vector<uint8_t>& key);




void rc5EncryptBlock(const Rc5KeySchedule& sched, uint32_t& A, uint32_t& B);


void rc5DecryptBlock(const Rc5KeySchedule& sched, uint32_t& A, uint32_t& B);




vector<uint8_t> rc5EncryptCBC(const Rc5KeySchedule& sched,
                               const vector<uint8_t>& plaintext,
                               const vector<uint8_t>& iv);


vector<uint8_t> rc5DecryptCBC(const Rc5KeySchedule& sched,
                               const vector<uint8_t>& ciphertext,
                               const vector<uint8_t>& iv);


void runRC5();
