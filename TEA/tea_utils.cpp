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
    return dir + "/" + "encrypted_" + extractFilename(sourcePath);
}

string buildDecryptPath(const string& sourcePath) {
    const string dir = "Decryptfiles";
    ensureDir(dir);

    string filename = extractFilename(sourcePath);

    // Убираем префикс "encrypted_" если он есть
    const string prefix = "encrypted_";
    if (filename.size() > prefix.size() &&
        filename.substr(0, prefix.size()) == prefix)
    {
        filename = filename.substr(prefix.size());
    }

    return dir + "/" + "decrypted_" + filename;
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