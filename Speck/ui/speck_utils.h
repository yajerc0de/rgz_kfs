#pragma once

#include <string>
#include <vector>
#include <cstdint>

bool speck_hexToBytes(const std::string& hex, std::vector<uint8_t>& out);
std::string speck_bytesToHex(const std::vector<uint8_t>& data);
std::vector<uint8_t> speck_randomBytes(size_t count);
bool speck_readFile(const std::string& path, std::vector<uint8_t>& data);
bool speck_writeFile(const std::string& path, const std::vector<uint8_t>& data);
bool speck_ensureDir(const std::string& dirPath);
std::string speck_extractFilename(const std::string& path);
std::string speck_buildEncryptPath(const std::string& sourcePath);
std::string speck_buildDecryptPath(const std::string& originalName);
std::vector<uint8_t> speck_packFilenameHeader(const std::string& originalFilename);
bool speck_unpackFilenameHeader(const std::vector<uint8_t>& data,
                                 std::string& originalFilename,
                                 size_t& bytesConsumed);
std::vector<uint8_t> speck_generateAndSave(const std::string& filepath,
                                            size_t byteCount,
                                            const std::string& label);
std::vector<uint8_t> speck_loadFromFile(const std::string& filepath,
                                         const std::string& label);
