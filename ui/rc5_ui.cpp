#include "rc5_module_api.h"
#include "rc5_utils.h"

#include <iostream>
#include <string>
#include <limits>

using namespace std;



static const string KEY_FILE = "rc5_key.bin";
static const string IV_FILE  = "rc5_iv.bin";


#ifdef _WIN32
static const string LIB_PATH = "rc5.dll";
#elif defined(__APPLE__)
static const string LIB_PATH = "./rc5.dylib";
#else
static const string LIB_PATH = "./rc5.so";
#endif

static void printSep(char ch = '-', int w = 50) {
    cout << string(w, ch) << "\n";
}



static void modeText(Rc5Module& mod, Rc5Handle handle) {
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

        vector<uint8_t> iv = rc5_generateAndSave(IV_FILE, RC5_BLOCK_BYTES, "IV");
        if (iv.empty()) return;

        vector<uint8_t> plain(text.begin(), text.end());

        uint8_t* outData = nullptr;
        size_t   outLen  = 0;

        int ok = mod.encryptCbc(handle, plain.data(), plain.size(),
                                 iv.data(), &outData, &outLen);

        if (!ok) {
            cout << "\n  [!] Ошибка шифрования.\n";
            return;
        }

        vector<uint8_t> cipher(outData, outData + outLen);
        mod.freeBuffer(outData); // обязательно освобождаем буфер из библиотеки

        cout << "\n";
        printSep();
        cout << "  Шифротекст (HEX): " << rc5_bytesToHex(cipher) << "\n";
        cout << "  IV сохранён в   : " << IV_FILE  << "\n";
        cout << "  Ключ сохранён в : " << KEY_FILE << "\n";
        printSep();

    } else if (choice == 2) {
        cout << "\n  Введите шифротекст (HEX): ";
        string hexCipher;
        cin >> hexCipher;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        vector<uint8_t> iv = rc5_loadFromFile(IV_FILE, "IV");
        if (iv.empty() || iv.size() != RC5_BLOCK_BYTES) {
            cout << "\n  [!] Неверный IV в файле " << IV_FILE << "\n";
            return;
        }

        vector<uint8_t> cipher;
        if (!rc5_hexToBytes(hexCipher, cipher)) {
            cout << "\n  [!] Неверный формат шифротекста.\n";
            return;
        }

        uint8_t* outData = nullptr;
        size_t   outLen  = 0;

        int ok = mod.decryptCbc(handle, cipher.data(), cipher.size(),
                                 iv.data(), &outData, &outLen);

        if (!ok) {
            cout << "\n  [!] Ошибка расшифровки.\n";
            cout << "  (Возможно, неверный ключ, IV или повреждённые данные)\n";
            return;
        }

        vector<uint8_t> plain(outData, outData + outLen);
        mod.freeBuffer(outData);

        cout << "\n";
        printSep();
        cout << "  Результат: " << string(plain.begin(), plain.end()) << "\n";
        printSep();

    } else {
        cout << "\n  [!] Неверный выбор.\n";
    }
}



static void modeFile(Rc5Module& mod, Rc5Handle handle) {
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
        if (!rc5_readFile(inPath, plain)) return;

        
        string outPath = rc5_buildEncryptPath(inPath);

        vector<uint8_t> iv = rc5_generateAndSave(IV_FILE, RC5_BLOCK_BYTES, "IV");
        if (iv.empty()) return;

        uint8_t* outData = nullptr;
        size_t   outLen  = 0;

        int ok = mod.encryptCbc(handle, plain.data(), plain.size(),
                                 iv.data(), &outData, &outLen);

        if (!ok) {
            cout << "\n  [!] Ошибка шифрования файла.\n";
            return;
        }

        vector<uint8_t> cipher(outData, outData + outLen);
        mod.freeBuffer(outData);

        
        string originalName = rc5_extractFilename(inPath);
        vector<uint8_t> nameHeader = rc5_packFilenameHeader(originalName);

        
        vector<uint8_t> output;
        output.insert(output.end(), nameHeader.begin(), nameHeader.end());
        output.insert(output.end(), iv.begin(),         iv.end());
        output.insert(output.end(), cipher.begin(),     cipher.end());

        if (!rc5_writeFile(outPath, output)) return;

        cout << "\n";
        printSep();
        cout << "  Исходный файл     : " << inPath        << "\n";
        cout << "  Оригинальное имя  : " << originalName  << " (сохранено внутри .bin)\n";
        cout << "  Зашифрован в      : " << outPath       << "\n";
        cout << "  Исходный размер   : " << plain.size()  << " байт\n";
        cout << "  Итоговый размер   : " << output.size() << " байт\n";
        cout << "  Ключ сохранён в   : " << KEY_FILE      << "\n";
        cout << "  IV сохранён в     : " << IV_FILE       << "\n";
        printSep();

    } else if (choice == 2) {
        
        cout << "\n  Путь к зашифрованному файлу: ";
        string inPath;
        getline(cin, inPath);

        vector<uint8_t> raw;
        if (!rc5_readFile(inPath, raw)) return;

        
        string originalName;
        size_t headerSize = 0;
        if (!rc5_unpackFilenameHeader(raw, originalName, headerSize)) {
            cout << "\n  [!] Не удалось прочитать имя файла из метаданных.\n";
            cout << "  Файл повреждён или не был зашифрован этой программой.\n";
            return;
        }

        
        if (raw.size() < headerSize + static_cast<size_t>(RC5_BLOCK_BYTES * 2)) {
            cout << "\n  [!] Файл слишком мал или повреждён.\n";
            return;
        }

        vector<uint8_t> iv(raw.begin() + headerSize,
                            raw.begin() + headerSize + RC5_BLOCK_BYTES);
        vector<uint8_t> cipher(raw.begin() + headerSize + RC5_BLOCK_BYTES,
                                raw.end());

        
        string outPath = rc5_buildDecryptPath(originalName);

        cout << "  [ИМЯ] Восстановлено из метаданных: " << originalName << "\n";
        cout << "  [IV]  Извлечён из файла\n";
        cout << "  [IV]  HEX: " << rc5_bytesToHex(iv) << "\n";

        uint8_t* outData = nullptr;
        size_t   outLen  = 0;

        int ok = mod.decryptCbc(handle, cipher.data(), cipher.size(),
                                 iv.data(), &outData, &outLen);

        if (!ok) {
            cout << "\n  [!] Ошибка расшифровки файла.\n";
            cout << "  (Возможно, неверный ключ или повреждённый файл)\n";
            return;
        }

        vector<uint8_t> plain(outData, outData + outLen);
        mod.freeBuffer(outData);

        if (!rc5_writeFile(outPath, plain)) return;

        cout << "\n";
        printSep();
        cout << "  Зашифрованный файл: " << inPath       << "\n";
        cout << "  Расшифрован в     : " << outPath       << "\n";
        cout << "  Размер данных     : " << plain.size()  << " байт\n";
        printSep();

    } else {
        cout << "\n  [!] Неверный выбор.\n";
    }
}



static void modeKeyGen() {
    cout << "\n  Генератор ключей RC5\n";
    cout << "  Доступные варианты длины ключа:\n";
    cout << "    1. 16 байт (128 бит) — рекомендуется\n";
    cout << "    2. 24 байта (192 бит)\n";
    cout << "    3. 32 байта (256 бит)\n";
    cout << "  Выбор: ";

    int choice = 0;
    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    size_t keyLen = RC5_KEY_BYTES; // 16 байт по умолчанию
    switch (choice) {
        case 1: keyLen = 16; break;
        case 2: keyLen = 24; break;
        case 3: keyLen = 32; break;
        default:
            cout << "\n  [!] Неверный выбор. Используется 16 байт.\n";
            break;
    }

    vector<uint8_t> key = rc5_generateAndSave(KEY_FILE, keyLen, "КЛЮЧ");
    if (key.empty()) return;

    cout << "\n";
    printSep();
    cout << "  Файл : " << KEY_FILE << "\n";
    cout << "  Длина: " << keyLen << " байт (" << keyLen * 8 << " бит)\n";
    printSep();
}



void runRC5() {
    cout << "\n";
    printSep('=');
    cout << "  RC5 — Rivest Cipher 5\n";
    cout << "  Блок: 64 бит | Ключ: 128/192/256 бит | Раундов: 12 | Режим: CBC\n";
    printSep('=');

    // ── Загружаем динамическую библиотеку ────────────────────────────────────
    Rc5Module mod;
    if (!rc5ModuleLoad(mod, LIB_PATH)) {
        cout << "\n  [!] Не удалось загрузить библиотеку: " << LIB_PATH << "\n";
        cout << "  Причина: " << mod.lastError << "\n";
        cout << "  Убедитесь что " << LIB_PATH << " находится рядом с программой.\n";
        return;
    }
    cout << "  [OK] Библиотека " << LIB_PATH << " загружена.\n";

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

        
        vector<uint8_t> keyBytes = rc5_loadFromFile(KEY_FILE, "КЛЮЧ");
        if (keyBytes.empty()) {
            cout << "\n  [!] Файл ключа не найден: " << KEY_FILE << "\n";
            cout << "  Сначала сгенерируйте ключ (пункт 3).\n";
            continue;
        }

        
        Rc5Handle handle = mod.create();
        if (handle == nullptr) {
            cout << "\n  [!] Не удалось создать объект RC5 в библиотеке.\n";
            continue;
        }

        if (!mod.setKey(handle, keyBytes.data(), keyBytes.size())) {
            cout << "\n  [!] Неверная длина ключа в файле " << KEY_FILE << "\n";
            cout << "  RC5 принимает ключи от 1 до 255 байт.\n";
            mod.destroy(handle);
            continue;
        }
        cout << "  [OK] Ключ загружен.\n";

        if (choice == 1) modeText(mod, handle);
        else             modeFile(mod, handle);

       
        mod.destroy(handle);
    }
}