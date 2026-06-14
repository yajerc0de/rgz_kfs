#include "blowfish_utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

// Платформозависимое создание папки
#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
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
    // Проверяем существование через ifstream-трюк — без filesystem
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(dirPath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
        return true; // папка уже есть
    if (_mkdir(dirPath.c_str()) == 0) {
        cout << "  [*] Создана папка: " << dirPath << "\n";
        return true;
    }
#else
    struct stat st;
    if (stat(dirPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
        return true; // папка уже есть
    if (mkdir(dirPath.c_str(), 0755) == 0) {
        cout << "  [*] Создана папка: " << dirPath << "\n";
        return true;
    }
#endif
    cout << "\n  [!] Не удалось создать папку: " << dirPath << "\n";
    return false;
}

// =============================================================================
//  Работа с путями и именами файлов — без <filesystem>
// =============================================================================

string extractFilename(const string& path) {
    // Ищем последний '/' или '\' и берём всё после него
    size_t pos = path.find_last_of("/\\");
    if (pos == string::npos) return path;
    return path.substr(pos + 1);
}

string extractExtension(const string& path) {
    string filename = extractFilename(path);
    size_t dot = filename.find_last_of('.');
    if (dot == string::npos) return "";
    return filename.substr(dot); // включая точку, например ".jpg"
}

string buildEncryptPath(const string& sourcePath) {
    const string dir = "Encryptfiles";
    ensureDir(dir);
    string filename = extractFilename(sourcePath);
    return dir + "/" + "encrypted_" + filename;
}

string buildDecryptPath(const string& sourcePath) {
    const string dir = "Decryptfiles";
    ensureDir(dir);
    string filename = extractFilename(sourcePath);

    // Убираем префикс "encrypted_" чтобы восстановить оригинальное имя
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