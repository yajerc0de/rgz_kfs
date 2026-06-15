#include "tea.h"
#include "tea_utils.h"

#include <iostream>
#include <string>
#include <limits>

using namespace std;

// =============================================================================
//  tea_ui.cpp — консольный интерфейс модуля TEA
//  Утилиты (hex, файлы, random) переиспользуются из blowfish_utils
// =============================================================================

static const string KEY_FILE = "tea_key.bin";
static const string IV_FILE  = "tea_iv.bin";

static void printSep(char ch = '-', int w = 50) {
    cout << string(w, ch) << "\n";
}

// =============================================================================
//  Режим 1: шифрование / дешифрование текста
// =============================================================================

static void modeText(TEA& tea) {
    cout << "\n  Текстовый режим:\n";
    cout << "    1. Зашифровать текст\n";
    cout << "    2. Расшифровать текст\n";
    cout << "  Выбор: ";

    int choice = 0;
    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 1) {
        cout << "\n  Введите текст для шифрования:\n  > ";
        string text;
        getline(cin, text);

        vector<uint8_t> iv = generateAndSave(IV_FILE, TEA::BLOCK_BYTES, "IV");
        if (iv.empty()) return;

        vector<uint8_t> plain(text.begin(), text.end());

        try {
            vector<uint8_t> cipher = tea.encryptCBC(plain, iv);

            cout << "\n";
            printSep();
            cout << "  Шифротекст (HEX): " << bytesToHex(cipher) << "\n";
            cout << "  IV сохранён в   : " << IV_FILE  << "\n";
            cout << "  Ключ сохранён в : " << KEY_FILE << "\n";
            printSep();
        } catch (const exception& e) {
            cout << "\n  [!] Ошибка шифрования: " << e.what() << "\n";
        }

    } else if (choice == 2) {
        cout << "\n  Введите шифротекст (HEX): ";
        string hexCipher;
        cin >> hexCipher;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        vector<uint8_t> iv = loadFromFile(IV_FILE, "IV");
        if (iv.empty() || iv.size() != TEA::BLOCK_BYTES) {
            cout << "\n  [!] Неверный IV в файле " << IV_FILE << "\n";
            return;
        }

        vector<uint8_t> cipher;
        if (!hexToBytes(hexCipher, cipher)) {
            cout << "\n  [!] Неверный формат шифротекста.\n";
            return;
        }

        try {
            vector<uint8_t> plain = tea.decryptCBC(cipher, iv);
            cout << "\n";
            printSep();
            cout << "  Результат: " << string(plain.begin(), plain.end()) << "\n";
            printSep();
        } catch (const exception& e) {
            cout << "\n  [!] Ошибка расшифровки: " << e.what() << "\n";
        }

    } else {
        cout << "\n  [!] Неверный выбор.\n";
    }
}

// =============================================================================
//  Режим 2: шифрование / дешифрование файла
// =============================================================================

static void modeFile(TEA& tea) {
    cout << "\n  Файловый режим:\n";
    cout << "    1. Зашифровать файл\n";
    cout << "    2. Расшифровать файл\n";
    cout << "  Выбор: ";

    int choice = 0;
    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 1) {
        cout << "\n  Путь к исходному файлу: ";
        string inPath;
        getline(cin, inPath);

        vector<uint8_t> plain;
        if (!readFile(inPath, plain)) return;

        string outPath = buildEncryptPath(inPath);

        vector<uint8_t> iv = generateAndSave(IV_FILE, TEA::BLOCK_BYTES, "IV");
        if (iv.empty()) return;

        try {
            vector<uint8_t> cipher = tea.encryptCBC(plain, iv);

            // Формат файла: [8 байт IV][шифротекст]
            vector<uint8_t> output;
            output.insert(output.end(), iv.begin(),     iv.end());
            output.insert(output.end(), cipher.begin(), cipher.end());

            if (!writeFile(outPath, output)) return;

            cout << "\n";
            printSep();
            cout << "  Исходный файл   : " << inPath        << "\n";
            cout << "  Зашифрован в    : " << outPath        << "\n";
            cout << "  Исходный размер : " << plain.size()   << " байт\n";
            cout << "  Итоговый размер : " << output.size()  << " байт\n";
            cout << "  Ключ сохранён в : " << KEY_FILE       << "\n";
            cout << "  IV сохранён в   : " << IV_FILE        << "\n";
            printSep();
        } catch (const exception& e) {
            cout << "\n  [!] Ошибка: " << e.what() << "\n";
        }

    } else if (choice == 2) {
        cout << "\n  Путь к зашифрованному файлу: ";
        string inPath;
        getline(cin, inPath);

        vector<uint8_t> raw;
        if (!readFile(inPath, raw)) return;

        if (raw.size() < static_cast<size_t>(TEA::BLOCK_BYTES * 2)) {
            cout << "\n  [!] Файл слишком мал или повреждён.\n";
            return;
        }

        string outPath = buildDecryptPath(inPath);

        // IV извлекаем из первых 8 байт файла
        vector<uint8_t> iv(raw.begin(), raw.begin() + TEA::BLOCK_BYTES);
        vector<uint8_t> cipher(raw.begin() + TEA::BLOCK_BYTES, raw.end());

        cout << "  [IV] Извлечён из файла (первые 8 байт)\n";
        cout << "  [IV] HEX: " << bytesToHex(iv) << "\n";

        try {
            vector<uint8_t> plain = tea.decryptCBC(cipher, iv);
            if (!writeFile(outPath, plain)) return;

            cout << "\n";
            printSep();
            cout << "  Зашифрованный файл: " << inPath      << "\n";
            cout << "  Расшифрован в     : " << outPath      << "\n";
            cout << "  Размер данных     : " << plain.size() << " байт\n";
            printSep();
        } catch (const exception& e) {
            cout << "\n  [!] Ошибка: " << e.what()
                 << "\n  (Возможно, неверный ключ или повреждённый файл)\n";
        }

    } else {
        cout << "\n  [!] Неверный выбор.\n";
    }
}

// =============================================================================
//  Режим 3: генератор ключей
//  TEA всегда использует ровно 16 байт (128 бит) — длина фиксирована
// =============================================================================

static void modeKeyGen() {
    cout << "\n  Генератор ключей TEA\n";
    cout << "  Длина ключа фиксирована: "
         << TEA::KEY_BYTES << " байт ("
         << TEA::KEY_BYTES * 8 << " бит)\n\n";

    vector<uint8_t> key = generateAndSave(KEY_FILE, TEA::KEY_BYTES, "КЛЮЧ");
    if (key.empty()) return;

    cout << "\n";
    printSep();
    cout << "  Файл : " << KEY_FILE << "\n";
    cout << "  Длина: " << TEA::KEY_BYTES << " байт ("
         << TEA::KEY_BYTES * 8 << " бит)\n";
    printSep();
}

// =============================================================================
//  runTEA() — точка входа из main.cpp
// =============================================================================

void runTEA() {
    cout << "\n";
    printSep('=');
    cout << "  TEA — Tiny Encryption Algorithm\n";
    cout << "  Блок: 64 бит | Ключ: 128 бит (фикс.) | Раундов: 64 | Режим: CBC\n";
    printSep('=');

    bool running = true;
    while (running) {
        cout << "\n  Что вы хотите сделать?\n\n";
        cout << "    1. Зашифровать / расшифровать текст\n";
        cout << "    2. Зашифровать / расшифровать файл\n";
        cout << "    3. Сгенерировать ключ\n";
        cout << "    0. Вернуться в главное меню\n\n";
        printSep();
        cout << "  Выбор: ";

        int choice = 0;
        cin >> choice;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (choice == 0) {
            running = false;
            continue;
        }

        if (choice == 3) {
            modeKeyGen();
            continue;
        }

        if (choice != 1 && choice != 2) {
            cout << "\n  [!] Неверный выбор.\n";
            continue;
        }

        // Загружаем ключ из файла
        vector<uint8_t> keyBytes = loadFromFile(KEY_FILE, "КЛЮЧ");
        if (keyBytes.empty()) {
            cout << "\n  [!] Файл ключа не найден: " << KEY_FILE << "\n";
            cout << "  Сначала сгенерируйте ключ (пункт 3).\n";
            continue;
        }

        TEA tea;
        if (!tea.setKey(keyBytes)) {
            cout << "\n  [!] Неверная длина ключа в файле " << KEY_FILE << "\n";
            cout << "  TEA требует ровно " << TEA::KEY_BYTES << " байт.\n";
            continue;
        }
        cout << "  [OK] Ключ загружен.\n";

        if (choice == 1) modeText(tea);
        else             modeFile(tea);
    }
}