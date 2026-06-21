#pragma once

#include <string>
#include <vector>
#include <cstdint>

bool rc5_hexToBytes(const std::string& hex, std::vector<uint8_t>& out);
std::string rc5_bytesToHex(const std::vector<uint8_t>& data);
std::vector<uint8_t> rc5_randomBytes(size_t count);
bool rc5_readFile(const std::string& path, std::vector<uint8_t>& data);
bool rc5_writeFile(const std::string& path, const std::vector<uint8_t>& data);
bool rc5_ensureDir(const std::string& dirPath);
std::string rc5_extractFilename(const std::string& path);
std::string rc5_buildEncryptPath(const std::string& sourcePath);
std::string rc5_buildDecryptPath(const std::string& originalName);
std::vector<uint8_t> rc5_packFilenameHeader(const std::string& originalFilename);
bool rc5_unpackFilenameHeader(const std::vector<uint8_t>& data,
                               std::string& originalFilename,
                               size_t& bytesConsumed);
std::vector<uint8_t> rc5_generateAndSave(const std::string& filepath,
                                          size_t byteCount,
                                          const std::string& label);
std::vector<uint8_t> rc5_loadFromFile(const std::string& filepath,
                                       const std::string& label);
