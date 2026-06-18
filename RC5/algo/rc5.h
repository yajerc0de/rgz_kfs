#pragma once

#include <cstdint>
#include <vector>
#include <string>

using namespace std;



class RC5 {
public:
    
    static constexpr int      WORD_BITS    = 32;            
    static constexpr uint32_t W            = WORD_BITS;
    static constexpr int      ROUNDS       = 12;            
    static constexpr int      KEY_BYTES    = 16;            
    static constexpr int      BLOCK_BYTES  = 8;             
    static constexpr int      TABLE_SIZE   = 2 * (ROUNDS + 1); 

   
   
    static constexpr uint32_t P32 = 0xB7E15163u;
    static constexpr uint32_t Q32 = 0x9E3779B9u;

    

    RC5();

    
    bool setKey(const vector<uint8_t>& key);

   

    
    void encryptBlock(uint32_t& A, uint32_t& B) const;

   
    void decryptBlock(uint32_t& A, uint32_t& B) const;

   

    
    vector<uint8_t> encryptCBC(const vector<uint8_t>& plaintext,
                                const vector<uint8_t>& iv) const;

    
    vector<uint8_t> decryptCBC(const vector<uint8_t>& ciphertext,
                                const vector<uint8_t>& iv) const;

private:
   

    uint32_t m_S[TABLE_SIZE];   
    bool     m_keyIsSet = false;

    

    
    void expandKey(const vector<uint8_t>& key);

    
    static uint32_t rotl(uint32_t x, uint32_t s);

   
    static uint32_t rotr(uint32_t x, uint32_t s);

   
    static vector<uint8_t> pkcs7Pad(const vector<uint8_t>& data);

    
    static vector<uint8_t> pkcs7Unpad(const vector<uint8_t>& data);

    
    static void packBlock(uint32_t A, uint32_t B, uint8_t* out);

    
    static void unpackBlock(const uint8_t* in, uint32_t& A, uint32_t& B);
};


void runRC5();
