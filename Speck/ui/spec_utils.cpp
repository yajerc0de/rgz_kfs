#include "speck_utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
    #include <direct.h>
    #include <sys/stat.h>
#else
    #include <sys/stat.h>
#endif

using namespace std;

bool hexToBytes(const string& hex, vector<uint8_t>& out) {
    if (hex.size() % 2 != 0) return false;
    out.clear();
    for (size_t i = 0; i < hex.size(); i += 2) {
        try {
            out.push_back(static_cast<uint8_t>(stoul(hex.substr(i, 2), nullptr, 16)));
        } catch (...) {
            return false;
        }
    }
    return true;
}

string bytesToHex(const vector<uint8_t>& data) {
    ostringstream oss;
    for (uint8_t b : data)
        oss << hex << setw(2) << setfill('0') << static_cast<int>(b);
    return oss.str();
}

vector<uint8_t> randomBytes(size_t count) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(0, 255);
    vector<uint8_t> res(count);
    for (size_t i = 0; i < count; ++i) {
        res[i] = static_cast<uint8_t>(dist(gen));
    }
    return res;
}

bool readFile(const string& path, vector<uint8_t>& data) {
    ifstream ifs(path, ios::binary | ios::ate);
    if (!ifs.is_open()) return false;
    streamsize size = ifs.tellg();
    ifs.seekg(0, ios::beg);
    data.resize(size);
    if (ifs.read(reinterpret_cast<char*>(data.data()), size)) return true;
    return false;
}

bool writeFile(const string& path, const vector<uint8_t>& data) {
    ofstream ofs(path, ios::binary);
    if (!ofs.is_open()) return false;
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
    return ofs.good();
}

bool ensureDir(const string& dirPath) {
#ifdef _WIN32
    int res = _mkdir(dirPath.c_str());
#else
    int res = mkdir(dirPath.c_str(), 0777);
#endif
    if (res == 0) return true;
    struct stat info;
    if (stat(dirPath.c_str(), &info) == 0 && (info.st_mode & S_IFDIR)) return true;
    return false;
}

string extractFilename(const string& path) {
    size_t idx = path.find_last_of("\\/");
    if (idx == string::npos) return path;
    return path.substr(idx + 1);
}

string buildEncryptPath(const string& sourcePath) {
    ensureDir("Encryptfiles");
    return "Encryptfiles/" + extractFilename(sourcePath) + ".enc";
}

string buildDecryptPath(const string& originalName) {
    ensureDir("Decryptfiles");
    return "Decryptfiles/" + originalName;
}

vector<uint8_t> packFilenameHeader(const string& originalFilename) {
    uint32_t nameLen = static_cast<uint32_t>(originalFilename.size());
    vector<uint8_t> header(4 + nameLen);
    header[0] = nameLen & 0xFF;
    header[1] = (nameLen >> 8) & 0xFF;
    header[2] = (nameLen >> 16) & 0xFF;
    header[3] = (nameLen >> 24) & 0xFF;
    memcpy(header.data() + 4, originalFilename.data(), nameLen);
    return header;
}

bool unpackFilenameHeader(const vector<uint8_t>& data, string& originalFilename, size_t& bytesConsumed) {
    if (data.size() < 4) return false;
    uint32_t nameLen = static_cast<uint32_t>(data[0])
                      | (static_cast<uint32_t>(data[1]) << 8)
                      | (static_cast<uint32_t>(data[2]) << 16)
                      | (static_cast<uint32_t>(data[3]) << 24);
    const uint32_t MAX_NAME_LEN = 4096;
    if (nameLen == 0 || nameLen > MAX_NAME_LEN) return false;
    if (data.size() < 4 + static_cast<size_t>(nameLen)) return false;
    originalFilename.assign(data.begin() + 4, data.begin() + 4 + nameLen);
    bytesConsumed = 4 + nameLen;
    return true;
}

vector<uint8_t> generateAndSave(const string& filepath, size_t byteCount, const string& label) {
    vector<uint8_t> data = randomBytes(byteCount);
    if (!writeFile(filepath, data)) return {};
    cout << "  [" << label << "] Сгенерирован и сохранён в: " << filepath << "\n";
    cout << "  [" << label << "] HEX: " << bytesToHex(data) << "\n";
    return data;
}

vector<uint8_t> loadFromFile(const string& filepath, const string& label) {
    vector<uint8_t> data;
    if (!readFile(filepath, data)) return {};
    cout << "  [" << label << "] Успешно загружен из: " << filepath << "\n";
    cout << "  [" << label << "] HEX: " << bytesToHex(data) << "\n";
    return data;
}