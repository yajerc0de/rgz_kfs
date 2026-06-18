#include "tea_utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

#ifdef _WIN32
    #include <direct.h>   // _mkdir
    #include <sys/stat.h> // stat
#else
    #include <sys/stat.h> // mkdir, stat
#endif

using namespace std;

namespace TeaUtils {

// =============================================================================
//  HEX
// =============================================================================

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

// =============================================================================
//  Случайные байты
// =============================================================================

vector<uint8_t> randomBytes(size_t count) {
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> dist(0, 255);

    vector<uint8_t> result(count);
    for (auto& b : result)
        b = static_cast<uint8_t>(dist(rng));
    return result;
}

// =============================================================================
//  Файлы
// =============================================================================

bool readFile(const string& path, vector<uint8_t>& data) {
    ifstream f(path, ios::binary);
    if (!f) {
        cout << "\n  [!] Не удалось открыть файл: " << path << "\n";
        return false;
    }
    data.assign(istreambuf_iterator<char>(f), istreambuf_iterator<char>());
    return true;
}

bool writeFile(const string& path, const vector<uint8_t>& data) {
    ofstream f(path, ios::binary);
    if (!f) {
        cout << "\n  [!] Не удалось создать файл: " << path << "\n";
        return false;
    }
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

bool ensureDir(const string& dirPath) {
    // Проверяем существует ли папка через stat
    struct stat info;
    if (stat(dirPath.c_str(), &info) == 0 && (info.st_mode & S_IFDIR))
        return true;

    // Создаём папку
    int result = 0;
#ifdef _WIN32
    result = _mkdir(dirPath.c_str());
#else
    result = mkdir(dirPath.c_str(), 0755);
#endif

    if (result == 0) {
        cout << "  [*] Создана папка: " << dirPath << "\n";
        return true;
    }
    cout << "\n  [!] Не удалось создать папку: " << dirPath << "\n";
    return false;
}

// =============================================================================
//  Работа с путями — без filesystem, только строковые операции
// =============================================================================

string extractFilename(const string& path) {
    // Ищем последний слеш (прямой или обратный для Windows)
    size_t pos = path.find_last_of("/\\");
    if (pos == string::npos)
        return path;
    return path.substr(pos + 1);
}

string buildEncryptPath(const string& sourcePath) {
    const string dir = "Encryptfiles";
    ensureDir(dir);

    // Берём имя файла без расширения, добавляем .bin
    // Оригинальное расширение хранится внутри файла, а не в имени на диске
    string filename = extractFilename(sourcePath);

    size_t dotPos = filename.find_last_of('.');
    string baseName = (dotPos == string::npos) ? filename : filename.substr(0, dotPos);

    return dir + "/" + "encrypted_" + baseName + ".bin";
}

string buildDecryptPath(const string& originalName) {
    const string dir = "Decryptfiles";
    ensureDir(dir);

    // originalName уже извлечён из метаданных внутри .bin файла —
    // здесь просто собираем путь, без дополнительной обработки имени
    return dir + "/" + "decrypted_" + originalName;
}

// =============================================================================
//  Метаданные имени файла
//
//  Формат: [4 байта длина имени, little-endian][N байт имя файла UTF-8]
// =============================================================================

vector<uint8_t> packFilenameHeader(const string& originalFilename) {
    uint32_t nameLen = static_cast<uint32_t>(originalFilename.size());

    vector<uint8_t> header;
    header.reserve(4 + nameLen);

    // Длина имени — 4 байта little-endian
    header.push_back(static_cast<uint8_t>( nameLen        & 0xFF));
    header.push_back(static_cast<uint8_t>((nameLen >> 8)  & 0xFF));
    header.push_back(static_cast<uint8_t>((nameLen >> 16) & 0xFF));
    header.push_back(static_cast<uint8_t>((nameLen >> 24) & 0xFF));

    // Само имя файла
    header.insert(header.end(), originalFilename.begin(), originalFilename.end());

    return header;
}

bool unpackFilenameHeader(const vector<uint8_t>& data,
                           string& originalFilename,
                           size_t& bytesConsumed)
{
    if (data.size() < 4)
        return false;

    uint32_t nameLen = static_cast<uint32_t>(data[0])
                      | (static_cast<uint32_t>(data[1]) << 8)
                      | (static_cast<uint32_t>(data[2]) << 16)
                      | (static_cast<uint32_t>(data[3]) << 24);

    // Защита от повреждённых данных — разумный верхний предел длины имени
    const uint32_t MAX_NAME_LEN = 4096;
    if (nameLen == 0 || nameLen > MAX_NAME_LEN)
        return false;

    if (data.size() < 4 + static_cast<size_t>(nameLen))
        return false;

    originalFilename.assign(data.begin() + 4, data.begin() + 4 + nameLen);
    bytesConsumed = 4 + nameLen;
    return true;
}

// =============================================================================
//  Ключи и IV
// =============================================================================

vector<uint8_t> generateAndSave(const string& filepath, size_t byteCount, const string& label) {
    vector<uint8_t> data = randomBytes(byteCount);
    if (!writeFile(filepath, data))
        return {};
    cout << "  [" << label << "] Сгенерирован и сохранён в: " << filepath << "\n";
    cout << "  [" << label << "] HEX: " << bytesToHex(data) << "\n";
    return data;
}

vector<uint8_t> loadFromFile(const string& filepath, const string& label) {
    vector<uint8_t> data;
    if (!readFile(filepath, data))
        return {};
    cout << "  [" << label << "] Загружен из: " << filepath << "\n";
    cout << "  [" << label << "] HEX: " << bytesToHex(data) << "\n";
    return data;
}

} // namespace TeaUtils