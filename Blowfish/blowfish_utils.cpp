#include "blowfish_utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

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