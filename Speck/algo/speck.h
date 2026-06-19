#pragma once

#include <cstdint>
#include <vector>
#include <string>

constexpr int SPECK_BLOCK_BYTES = 16;

struct SpeckKeySchedule {
    uint64_t K[34];
    int rounds;
    bool keyIsSet;
};

void speckInitSchedule(SpeckKeySchedule& sched);

bool speckSetKey(SpeckKeySchedule& sched, const std::vector<uint8_t>& key);

void speckEncryptBlock(const SpeckKeySchedule& sched, uint64_t& x, uint64_t& y);

void speckDecryptBlock(const SpeckKeySchedule& sched, uint64_t& x, uint64_t& y);

std::vector<uint8_t> speckEncryptCBC(const SpeckKeySchedule& sched,
                                     const std::vector<uint8_t>& plaintext,
                                     const std::vector<uint8_t>& iv);

std::vector<uint8_t> speckDecryptCBC(const SpeckKeySchedule& sched,
                                     const std::vector<uint8_t>& ciphertext,
                                     const std::vector<uint8_t>& iv);

void runSpeck();