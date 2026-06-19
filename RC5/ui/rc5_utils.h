#pragma once

#include <string>
#include <vector>
#include <cstdint>

using namespace std;




bool hexToBytes(const string& hex, vector<uint8_t>& out);


string bytesToHex(const vector<uint8_t>& data);




vector<uint8_t> randomBytes(size_t count);




bool readFile(const string& path, vector<uint8_t>& data);


bool writeFile(const string& path, const vector<uint8_t>& data);


bool ensureDir(const string& dirPath);


string extractFilename(const string& path);


string buildEncryptPath(const string& sourcePath);


string buildDecryptPath(const string& originalName);




vector<uint8_t> packFilenameHeader(const string& originalFilename);


bool unpackFilenameHeader(const vector<uint8_t>& data,
                           string& originalFilename,
                           size_t& bytesConsumed);


vector<uint8_t> generateAndSave(const string& filepath, size_t byteCount, const string& label);


vector<uint8_t> loadFromFile(const string& filepath, const string& label);
