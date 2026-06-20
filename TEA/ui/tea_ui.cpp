#include "TEA/algo/tea.h"

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>

using namespace std;

// ─── HEX вывод ───────────────────────────────────────────────────────────────
static string to_hex(const vector<uint8_t>& data) {
    ostringstream oss;
    for (uint8_t b : data)
        oss << hex << uppercase << setw(2) << setfill('0') << (int)b;
    return oss.str();
}

static bool from_hex(const string& hex, vector<uint8_t>& out) {
    if (hex.size() % 2 != 0) return false;
    out.clear();
    for (size_t i = 0; i < hex.size(); i += 2) {
        try {
            out.push_back((uint8_t)stoul(hex.substr(i, 2), nullptr, 16));
        } catch (...) { return false; }
    }
    return true;
}

// ─── Чтение / запись файла ───────────────────────────────────────────────────
static bool read_file(const string& path, vector<uint8_t>& data) {
    ifstream f(path, ios::binary);
    if (!f) { cerr << "[!] Не удалось открыть: " << path << "\n"; return false; }
    data.assign(istreambuf_iterator<char>(f), istreambuf_iterator<char>());
    return true;
}

static bool write_file(const string& path, const vector<uint8_t>& data) {
    ofstream f(path, ios::binary);
    if (!f) { cerr << "[!] Не удалось создать: " << path << "\n"; return false; }
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

// ─── Ввод ключа вручную ──────────────────────────────────────────────────────
static bool input_key(TeaKey& tk) {
    cout << "Введите ключ в HEX (32 символа = 16 байт): ";
    string hexKey;
    cin >> hexKey;
    cin.ignore();

    vector<uint8_t> keyBytes;
    if (!from_hex(hexKey, keyBytes) || keyBytes.size() != 16) {
        cerr << "[!] Неверный ключ. Нужно ровно 32 HEX-символа.\n";
        return false;
    }

    tea_key_init(&tk);
    if (!tea_key_set(&tk, keyBytes.data(), (int)keyBytes.size())) {
        cerr << "[!] Ошибка установки ключа.\n";
        return false;
    }

    cout << "[OK] Ключ установлен.\n";
    return true;
}

// ─── Ввод IV вручную ─────────────────────────────────────────────────────────
static bool input_iv(vector<uint8_t>& iv) {
    cout << "Введите IV в HEX  (16 символов = 8 байт): ";
    string hexIV;
    cin >> hexIV;
    cin.ignore();

    if (!from_hex(hexIV, iv) || iv.size() != 8) {
        cerr << "[!] Неверный IV. Нужно ровно 16 HEX-символов.\n";
        return false;
    }

    cout << "[OK] IV установлен.\n";
    return true;
}

// ─── Режим 1: шифрование текста ──────────────────────────────────────────────
static void encrypt_text() {
    TeaKey tk;
    if (!input_key(tk)) return;

    vector<uint8_t> iv;
    if (!input_iv(iv)) return;

    cout << "Введите текст для шифрования: ";
    string text;
    getline(cin, text);

    vector<uint8_t> plain(text.begin(), text.end());
    vector<uint8_t> cipher = tea_cbc_encrypt(&tk, plain, iv.data());

    if (cipher.empty()) { cerr << "[!] Ошибка шифрования.\n"; return; }

    cout << "\n--------------------------------------------------\n";
    cout << "Шифротекст (HEX): " << to_hex(cipher) << "\n";
    cout << "--------------------------------------------------\n";
}

// ─── Режим 2: дешифрование текста ────────────────────────────────────────────
static void decrypt_text() {
    TeaKey tk;
    if (!input_key(tk)) return;

    vector<uint8_t> iv;
    if (!input_iv(iv)) return;

    cout << "Введите шифротекст (HEX): ";
    string hexCipher;
    cin >> hexCipher;
    cin.ignore();

    vector<uint8_t> cipher;
    if (!from_hex(hexCipher, cipher)) {
        cerr << "[!] Неверный HEX.\n"; return;
    }

    vector<uint8_t> plain = tea_cbc_decrypt(&tk, cipher, iv.data());

    if (plain.empty()) { cerr << "[!] Ошибка дешифрования.\n"; return; }

    cout << "\n--------------------------------------------------\n";
    cout << "Расшифровано: " << string(plain.begin(), plain.end()) << "\n";
    cout << "--------------------------------------------------\n";
}

// ─── Режим 3: шифрование файла ───────────────────────────────────────────────
static void encrypt_file() {
    TeaKey tk;
    if (!input_key(tk)) return;

    vector<uint8_t> iv;
    if (!input_iv(iv)) return;

    cout << "Путь к исходному файлу: ";
    string inPath;
    getline(cin, inPath);

    vector<uint8_t> plain;
    if (!read_file(inPath, plain)) return;

    vector<uint8_t> cipher = tea_cbc_encrypt(&tk, plain, iv.data());
    if (cipher.empty()) { cerr << "[!] Ошибка шифрования.\n"; return; }

    // Формат: [8 байт IV][шифротекст]
    vector<uint8_t> output(iv.begin(), iv.end());
    output.insert(output.end(), cipher.begin(), cipher.end());

    string outPath = inPath + ".tea";
    if (!write_file(outPath, output)) return;

    cout << "\n--------------------------------------------------\n";
    cout << "Зашифрован в   : " << outPath << "\n";
    cout << "Исходный размер: " << plain.size()  << " байт\n";
    cout << "Итоговый размер: " << output.size() << " байт\n";
    cout << "--------------------------------------------------\n";
}

// ─── Режим 4: дешифрование файла ─────────────────────────────────────────────
static void decrypt_file() {
    TeaKey tk;
    if (!input_key(tk)) return;

    cout << "Путь к зашифрованному файлу (.tea): ";
    string inPath;
    getline(cin, inPath);

    vector<uint8_t> raw;
    if (!read_file(inPath, raw)) return;

    if (raw.size() < 8) {
        cerr << "[!] Файл слишком мал.\n"; return;
    }

    // Первые 8 байт — IV
    vector<uint8_t> iv(raw.begin(), raw.begin() + 8);
    vector<uint8_t> cipher(raw.begin() + 8, raw.end());

    vector<uint8_t> plain = tea_cbc_decrypt(&tk, cipher, iv.data());
    if (plain.empty()) { cerr << "[!] Ошибка дешифрования.\n"; return; }

    // Убираем .tea из имени
    string outPath = inPath;
    if (outPath.size() > 4 && outPath.substr(outPath.size() - 4) == ".tea")
        outPath = outPath.substr(0, outPath.size() - 4);
    outPath = "decrypted_" + outPath;

    if (!write_file(outPath, plain)) return;

    cout << "\n--------------------------------------------------\n";
    cout << "Расшифрован в  : " << outPath      << "\n";
    cout << "Размер данных  : " << plain.size() << " байт\n";
    cout << "--------------------------------------------------\n";
}

// ─── main ─────────────────────────────────────────────────────────────────────
int main() {
    cout << "==================================================\n";
    cout << "  TEA standalone — без DLL, напрямую через .h\n";
    cout << "  Блок: 64 бит | Ключ: 128 бит | Режим: CBC\n";
    cout << "==================================================\n";

    bool running = true;
    while (running) {
        cout << "\n  Выберите операцию:\n";
        cout << "    1. Зашифровать текст\n";
        cout << "    2. Расшифровать текст\n";
        cout << "    3. Зашифровать файл\n";
        cout << "    4. Расшифровать файл\n";
        cout << "    0. Выход\n";
        cout << "  Выбор: ";

        int choice = 0;
        cin >> choice;
        cin.ignore();

        switch (choice) {
            case 1: encrypt_text(); break;
            case 2: decrypt_text(); break;
            case 3: encrypt_file(); break;
            case 4: decrypt_file(); break;
            case 0: running = false; break;
            default: cout << "[!] Неверный выбор.\n";
        }
    }

    return 0;
}