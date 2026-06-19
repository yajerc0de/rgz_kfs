#include "speck_utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <cstring>

#ifdef _WIN32
    #include <direct.h>
    #include <sys/stat.h>
#else
    #include <sys/stat.h>
#endif

bool speck_hexToBytes(const std::string& hex, std::vector<uint8_t>& out) {
    if (hex.size() % 2 != 0) return false;
    out.clear();
    for (size_t i = 0; i < hex.size(); i += 2) {
        try {
            out.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
        } catch (...) { return false; }
    }
    return true;
}

std::string speck_bytesToHex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (uint8_t b : data)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
}

std::vector<uint8_t> speck_randomBytes(size_t count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);
    std::vector<uint8_t> res(count);
    for (size_t i = 0; i < count; ++i) res[i] = static_cast<uint8_t>(dist(gen));
    return res;
}

bool speck_readFile(const std::string& path, std::vector<uint8_t>& data) {
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) return false;
    std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    data.resize(size);
    return (bool)ifs.read(reinterpret_cast<char*>(data.data()), size);
}

bool speck_writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) return false;
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
    return ofs.good();
}

bool speck_ensureDir(const std::string& dirPath) {
#ifdef _WIN32
    int res = _mkdir(dirPath.c_str());
#else
    int res = mkdir(dirPath.c_str(), 0777);
#endif
    if (res == 0) return true;
    struct stat info;
    return (stat(dirPath.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
}

std::string speck_extractFilename(const std::string& path) {
    size_t idx = path.find_last_of("\\/");
    return (idx == std::string::npos) ? path : path.substr(idx + 1);
}

std::string speck_buildEncryptPath(const std::string& sourcePath) {
    speck_ensureDir("Encryptfiles");
    return "Encryptfiles/" + speck_extractFilename(sourcePath) + ".enc";
}

std::string speck_buildDecryptPath(const std::string& originalName) {
    speck_ensureDir("Decryptfiles");
    return "Decryptfiles/" + originalName;
}

std::vector<uint8_t> speck_packFilenameHeader(const std::string& originalFilename) {
    uint32_t nameLen = static_cast<uint32_t>(originalFilename.size());
    std::vector<uint8_t> header(4 + nameLen);
    header[0] = nameLen & 0xFF;
    header[1] = (nameLen >> 8) & 0xFF;
    header[2] = (nameLen >> 16) & 0xFF;
    header[3] = (nameLen >> 24) & 0xFF;
    memcpy(header.data() + 4, originalFilename.data(), nameLen);
    return header;
}

bool speck_unpackFilenameHeader(const std::vector<uint8_t>& data,
                                 std::string& originalFilename,
                                 size_t& bytesConsumed) {
    if (data.size() < 4) return false;
    uint32_t nameLen = static_cast<uint32_t>(data[0])
                     | (static_cast<uint32_t>(data[1]) <<  8)
                     | (static_cast<uint32_t>(data[2]) << 16)
                     | (static_cast<uint32_t>(data[3]) << 24);
    if (nameLen == 0 || nameLen > 4096) return false;
    if (data.size() < 4 + static_cast<size_t>(nameLen)) return false;
    originalFilename.assign(data.begin() + 4, data.begin() + 4 + nameLen);
    bytesConsumed = 4 + nameLen;
    return true;
}

std::vector<uint8_t> speck_generateAndSave(const std::string& filepath,
                                            size_t byteCount,
                                            const std::string& label) {
    std::vector<uint8_t> data = speck_randomBytes(byteCount);
    if (!speck_writeFile(filepath, data)) return {};
    std::cout << "  [" << label << "] Сгенерирован и сохранён в: " << filepath << "\n";
    std::cout << "  [" << label << "] HEX: " << speck_bytesToHex(data) << "\n";
    return data;
}

std::vector<uint8_t> speck_loadFromFile(const std::string& filepath,
                                         const std::string& label) {
    std::vector<uint8_t> data;
    if (!speck_readFile(filepath, data)) return {};
    std::cout << "  [" << label << "] Успешно загружен из: " << filepath << "\n";
    std::cout << "  [" << label << "] HEX: " << speck_bytesToHex(data) << "\n";
    return data;
}
