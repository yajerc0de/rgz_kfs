#include "rc5_utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>

#ifdef _WIN32
    #include <direct.h>
    #include <sys/stat.h>
#else
    #include <sys/stat.h>
#endif

bool rc5_hexToBytes(const std::string& hex, std::vector<uint8_t>& out) {
    if (hex.size() % 2 != 0) return false;
    out.clear();
    for (size_t i = 0; i < hex.size(); i += 2) {
        try {
            out.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
        } catch (...) { return false; }
    }
    return true;
}

std::string rc5_bytesToHex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (uint8_t b : data)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
}

std::vector<uint8_t> rc5_randomBytes(size_t count) {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(0, 255);
    std::vector<uint8_t> result(count);
    for (auto& b : result) b = static_cast<uint8_t>(dist(rng));
    return result;
}

bool rc5_readFile(const std::string& path, std::vector<uint8_t>& data) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { std::cout << "\n  [!] Не удалось открыть файл: " << path << "\n"; return false; }
    data.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

bool rc5_writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) { std::cout << "\n  [!] Не удалось создать файл: " << path << "\n"; return false; }
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

bool rc5_ensureDir(const std::string& dirPath) {
    struct stat info;
    if (stat(dirPath.c_str(), &info) == 0 && (info.st_mode & S_IFDIR)) return true;
    int result = 0;
#ifdef _WIN32
    result = _mkdir(dirPath.c_str());
#else
    result = mkdir(dirPath.c_str(), 0755);
#endif
    if (result == 0) { std::cout << "  [*] Создана папка: " << dirPath << "\n"; return true; }
    std::cout << "\n  [!] Не удалось создать папку: " << dirPath << "\n";
    return false;
}

std::string rc5_extractFilename(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

std::string rc5_buildEncryptPath(const std::string& sourcePath) {
    const std::string dir = "Encryptfiles";
    rc5_ensureDir(dir);
    std::string filename = rc5_extractFilename(sourcePath);
    size_t dotPos = filename.find_last_of('.');
    std::string baseName = (dotPos == std::string::npos) ? filename : filename.substr(0, dotPos);
    return dir + "/encrypted_" + baseName + ".bin";
}

std::string rc5_buildDecryptPath(const std::string& originalName) {
    const std::string dir = "Decryptfiles";
    rc5_ensureDir(dir);
    return dir + "/decrypted_" + originalName;
}

std::vector<uint8_t> rc5_packFilenameHeader(const std::string& originalFilename) {
    uint32_t nameLen = static_cast<uint32_t>(originalFilename.size());
    std::vector<uint8_t> header;
    header.reserve(4 + nameLen);
    header.push_back(static_cast<uint8_t>( nameLen        & 0xFF));
    header.push_back(static_cast<uint8_t>((nameLen >>  8) & 0xFF));
    header.push_back(static_cast<uint8_t>((nameLen >> 16) & 0xFF));
    header.push_back(static_cast<uint8_t>((nameLen >> 24) & 0xFF));
    header.insert(header.end(), originalFilename.begin(), originalFilename.end());
    return header;
}

bool rc5_unpackFilenameHeader(const std::vector<uint8_t>& data,
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

std::vector<uint8_t> rc5_generateAndSave(const std::string& filepath,
                                          size_t byteCount,
                                          const std::string& label) {
    std::vector<uint8_t> data = rc5_randomBytes(byteCount);
    if (!rc5_writeFile(filepath, data)) return {};
    std::cout << "  [" << label << "] Сгенерирован и сохранён в: " << filepath << "\n";
    std::cout << "  [" << label << "] HEX: " << rc5_bytesToHex(data) << "\n";
    return data;
}

std::vector<uint8_t> rc5_loadFromFile(const std::string& filepath,
                                       const std::string& label) {
    std::vector<uint8_t> data;
    if (!rc5_readFile(filepath, data)) return {};
    std::cout << "  [" << label << "] Загружен из: " << filepath << "\n";
    std::cout << "  [" << label << "] HEX: " << rc5_bytesToHex(data) << "\n";
    return data;
}
