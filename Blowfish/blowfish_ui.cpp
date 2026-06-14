#include "blowfish.h"
#include "blowfish_utils.h"

#include <iostream>
#include <string>
#include <limits>

using namespace std;

// =============================================================================
//  blowfish_ui.cpp — консольный интерфейс модуля Blowfish
// =============================================================================

// Имена файлов для хранения ключа и IV (рядом с исполняемым файлом)
static const string KEY_FILE = "blowfish_key.bin";
static const string IV_FILE  = "blowfish_iv.bin";

static void printSep(char ch = '-', int w = 50) {
    cout << string(w, ch) << "\n";
}

// =============================================================================
//  Режим 1: шифрование / дешифрование текста
// =============================================================================

static void modeText(Blowfish& bf) {
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

        // Генерируем IV и сохраняем в файл
        vector<uint8_t> iv = generateAndSave(IV_FILE, Blowfish::BLOCK_BYTES, "IV");
        if (iv.empty()) return;

        vector<uint8_t> plain(text.begin(), text.end());

        try {
            vector<uint8_t> cipher = bf.encryptCBC(plain, iv);

            cout << "\n";
            printSep();
            cout << "  Шифротекст (HEX): " << bytesToHex(cipher) << "\n";
            cout << "  IV сохранён в   : " << IV_FILE << "\n";
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

        // Загружаем IV из файла
        vector<uint8_t> iv = loadFromFile(IV_FILE, "IV");
        if (iv.empty() || iv.size() != Blowfish::BLOCK_BYTES) {
            cout << "\n  [!] Неверный IV в файле " << IV_FILE << "\n";
            return;
        }

        vector<uint8_t> cipher;
        if (!hexToBytes(hexCipher, cipher)) {
            cout << "\n  [!] Неверный формат шифротекста.\n";
            return;
        }

        try {
            vector<uint8_t> plain = bf.decryptCBC(cipher, iv);
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

static void modeFile(Blowfish& bf) {
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

        cout << "  Путь для сохранения зашифрованного файла: ";
        string outPath;
        getline(cin, outPath);

        vector<uint8_t> plain;
        if (!readFile(inPath, plain)) return;

        // Генерируем IV и сохраняем в файл
        vector<uint8_t> iv = generateAndSave(IV_FILE, Blowfish::BLOCK_BYTES, "IV");
        if (iv.empty()) return;

        try {
            vector<uint8_t> cipher = bf.encryptCBC(plain, iv);

            // Формат файла: [8 байт IV][шифротекст]
            // IV дублируется в файл для удобства — основной хранится в blowfish_iv.bin
            vector<uint8_t> output;
            output.insert(output.end(), iv.begin(),     iv.end());
            output.insert(output.end(), cipher.begin(), cipher.end());

            if (!writeFile(outPath, output)) return;

            cout << "\n";
            printSep();
            cout << "  Файл зашифрован : " << outPath       << "\n";
            cout << "  Исходный размер : " << plain.size()  << " байт\n";
            cout << "  Итоговый размер : " << output.size() << " байт\n";
            cout << "  IV сохранён в   : " << IV_FILE       << "\n";
            cout << "  Ключ сохранён в : " << KEY_FILE      << "\n";
            printSep();
        } catch (const exception& e) {
            cout << "\n  [!] Ошибка: " << e.what() << "\n";
        }

    } else if (choice == 2) {
        cout << "\n  Путь к зашифрованному файлу: ";
        string inPath;
        getline(cin, inPath);

        cout << "  Путь для сохранения расшифрованного файла: ";
        string outPath;
        getline(cin, outPath);

        vector<uint8_t> raw;
        if (!readFile(inPath, raw)) return;

        if (raw.size() < static_cast<size_t>(Blowfish::BLOCK_BYTES * 2)) {
            cout << "\n  [!] Файл слишком мал или повреждён.\n";
            return;
        }

        // IV берём из первых 8 байт зашифрованного файла
        vector<uint8_t> iv(raw.begin(), raw.begin() + Blowfish::BLOCK_BYTES);
        vector<uint8_t> cipher(raw.begin() + Blowfish::BLOCK_BYTES, raw.end());

        cout << "  [IV] Извлечён из файла (первые 8 байт)\n";
        cout << "  [IV] HEX: " << bytesToHex(iv) << "\n";

        try {
            vector<uint8_t> plain = bf.decryptCBC(cipher, iv);
            if (!writeFile(outPath, plain)) return;

            cout << "\n";
            printSep();
            cout << "  Файл расшифрован: " << outPath      << "\n";
            cout << "  Размер данных   : " << plain.size() << " байт\n";
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
// =============================================================================

static void modeKeyGen() {
    cout << "\n  Генератор ключей Blowfish\n";
    cout << "  Введите желаемую длину ключа в байтах ("
         << Blowfish::KEY_MIN << "-" << Blowfish::KEY_MAX << "): ";

    int len = 0;
    cin >> len;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (len < Blowfish::KEY_MIN || len > Blowfish::KEY_MAX) {
        cout << "\n  [!] Недопустимая длина. Используем 32 байта.\n";
        len = 32;
    }

    vector<uint8_t> key = generateAndSave(KEY_FILE, static_cast<size_t>(len), "КЛЮЧ");
    if (key.empty()) return;

    cout << "\n";
    printSep();
    cout << "  Длина ключа : " << len << " байт (" << len * 8 << " бит)\n";
    cout << "  Файл        : " << KEY_FILE << "\n";
    printSep();
}

// =============================================================================
//  runBlowfish() — точка входа из main.cpp
// =============================================================================

void runBlowfish() {
    cout << "\n";
    printSep('=');
    cout << "  Blowfish — симметричный блочный шифр\n";
    cout << "  Блок: 64 бит | Ключ: 32-448 бит | Раундов: 16 | Режим: CBC\n";
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

        // Для режимов 1 и 2 — загружаем ключ из файла
        vector<uint8_t> keyBytes = loadFromFile(KEY_FILE, "КЛЮЧ");
        if (keyBytes.empty()) {
            cout << "\n  [!] Файл ключа не найден: " << KEY_FILE << "\n";
            cout << "  Сначала сгенерируйте ключ (пункт 3).\n";
            continue;
        }

        Blowfish bf;
        if (!bf.setKey(keyBytes)) {
            cout << "\n  [!] Неверная длина ключа в файле " << KEY_FILE << "\n";
            continue;
        }
        cout << "  [OK] Ключ загружен. Key Schedule выполнен.\n";

        if (choice == 1) modeText(bf);
        else             modeFile(bf);
    }
}