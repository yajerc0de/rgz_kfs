#include "speck_module_api.h"
#include "speck_utils.h"
#include <iostream>
#include <string>
#include <limits>

using namespace std;

static const string KEY_FILE = "speck_key.bin";
static const string IV_FILE  = "speck_iv.bin";

#ifdef _WIN32
static const string LIB_PATH = "speck.dll";
#else
static const string LIB_PATH = "./speck.so";
#endif

static void printSep(char ch = '-', int w = 50) {
    cout << string(w, ch) << "\n";
}

static void modeText(SpeckModule& mod, SpeckHandle handle) {
    cout << "\n  Текстовый режим:\n";
    cout << "    1. Зашифровать текст\n";
    cout << "    2. Расшифровать текст\n";
    cout << "    Выбор: ";
    int action = 0;
    cin >> action;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    vector<uint8_t> ivBytes = speck_loadFromFile(IV_FILE, "IV");
    if (ivBytes.empty()) {
        cout << "  [!] Ошибка загрузки IV. Сначала выполните генерацию параметров.\n";
        return;
    }

    if (action == 1) {
        cout << "  Введите строку для шифрования: ";
        string text;
        getline(cin, text);
        if (text.empty()) return;

        uint8_t* outBuf = nullptr;
        size_t outLen = 0;
        int ok = mod.encryptCbc(handle, reinterpret_cast<const uint8_t*>(text.data()), text.size(), ivBytes.data(), &outBuf, &outLen);
        if (!ok || outBuf == nullptr) {
            cout << "  [!] Ошибка выполнения операции шифрования.\n";
            return;
        }

        vector<uint8_t> encBytes(outBuf, outBuf + outLen);
        mod.freeBuffer(outBuf);

        string hexStr = speck_bytesToHex(encBytes);
        cout << "  Результат (HEX): " << hexStr << "\n";
    } 
    else if (action == 2) {
        cout << "  Введите HEX-строку для дешифрования: ";
        string hexStr;
        getline(cin, hexStr);
        vector<uint8_t> encBytes;
        if (!speck_hexToBytes(hexStr, encBytes)) {
            cout << "  [!] Некорректный формат HEX.\n";
            return;
        }

        uint8_t* outBuf = nullptr;
        size_t outLen = 0;
        int ok = mod.decryptCbc(handle, encBytes.data(), encBytes.size(), ivBytes.data(), &outBuf, &outLen);
        if (!ok || outBuf == nullptr) {
            cout << "  [!] Ошибка выполнения операции дешифрования (проверьте целостность).\n";
            return;
        }

        string plainText(reinterpret_cast<char*>(outBuf), outLen);
        mod.freeBuffer(outBuf);

        cout << "  Результат (Текст): " << plainText << "\n";
    }
}

static void modeFile(SpeckModule& mod, SpeckHandle handle) {
    cout << "\n  Файловый режим:\n";
    cout << "    1. Зашифровать файл\n";
    cout << "    2. Расшифровать файл\n";
    cout << "    Выбор: ";
    int action = 0;
    cin >> action;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    vector<uint8_t> ivBytes = speck_loadFromFile(IV_FILE, "IV");
    if (ivBytes.empty()) {
        cout << "  [!] Ошибка загрузки IV. Сначала выполните генерацию параметров.\n";
        return;
    }

    if (action == 1) {
        cout << "  Введите путь к исходному файлу: ";
        string srcPath;
        getline(cin, srcPath);
        vector<uint8_t> fileData;
        if (!speck_readFile(srcPath, fileData)) {
            cout << "  [!] Не удалось прочитать файл.\n";
            return;
        }

        string pureName = speck_extractFilename(srcPath);
        vector<uint8_t> header = speck_packFilenameHeader(pureName);
        vector<uint8_t> fullPayload;
        fullPayload.reserve(header.size() + fileData.size());
        fullPayload.insert(fullPayload.end(), header.begin(), header.end());
        fullPayload.insert(fullPayload.end(), fileData.begin(), fileData.end());

        uint8_t* outBuf = nullptr;
        size_t outLen = 0;
        int ok = mod.encryptCbc(handle, fullPayload.data(), fullPayload.size(), ivBytes.data(), &outBuf, &outLen);
        if (!ok || outBuf == nullptr) {
            cout << "  [!] Ошибка выполнения операции шифрования.\n";
            return;
        }

        vector<uint8_t> encData(outBuf, outBuf + outLen);
        mod.freeBuffer(outBuf);

        string destPath = speck_buildEncryptPath(srcPath);
        if (speck_writeFile(destPath, encData)) {
            cout << "  [OK] Файл успешно зашифрован и сохранён в: " << destPath << "\n";
        } else {
            cout << "  [!] Не удалось сохранить результат в файл.\n";
        }
    } 
    else if (action == 2) {
        cout << "  Введите путь к зашифрованному файлу (*.enc): ";
        string encPath;
        getline(cin, encPath);
        vector<uint8_t> encData;
        if (!speck_readFile(encPath, encData)) {
            cout << "  [!] Не удалось прочитать файл.\n";
            return;
        }

        uint8_t* outBuf = nullptr;
        size_t outLen = 0;
        int ok = mod.decryptCbc(handle, encData.data(), encData.size(), ivBytes.data(), &outBuf, &outLen);
        if (!ok || outBuf == nullptr) {
            cout << "  [!] Ошибка выполнения операции дешифрования.\n";
            return;
        }

        vector<uint8_t> fullPayload(outBuf, outBuf + outLen);
        mod.freeBuffer(outBuf);

        string origName;
        size_t bytesConsumed = 0;
        if (!speck_unpackFilenameHeader(fullPayload, origName, bytesConsumed)) {
            cout << "  [!] Ошибка разбора заголовка оригинального имени.\n";
            return;
        }

        vector<uint8_t> fileData(fullPayload.begin() + bytesConsumed, fullPayload.end());
        string destPath = speck_buildDecryptPath(origName);
        if (speck_writeFile(destPath, fileData)) {
            cout << "  [OK] Файл успешно расшифрован и сохранён под исходным именем: " << destPath << "\n";
        } else {
            cout << "  [!] Не удалось сохранить результат в файл.\n";
        }
    }
}

static void modeKeyGen() {
    cout << "\n  Генерация криптографических параметров:\n";
    cout << "    1. Ключ 128 бит (16 байт)\n";
    cout << "    2. Ключ 192 бита (24 байта)\n";
    cout << "    3. Ключ 256 бит (32 байта)\n";
    cout << "    Выбор длины ключа: ";
    int kChoice = 0;
    cin >> kChoice;
    
    size_t kBytes = 16;
    if (kChoice == 2) kBytes = 24;
    else if (kChoice == 3) kBytes = 32;

    speck_generateAndSave(KEY_FILE, kBytes, "КЛЮЧ");
    speck_generateAndSave(IV_FILE, 16, "IV");
}

void runSpeck() {
    SpeckModule mod;
    if (!speckModuleLoad(mod, LIB_PATH)) {
        cout << "  [!] Критическая ошибка загрузки модуля: " << LIB_PATH << "\n";
        cout << "  " << mod.lastError << "\n";
        return;
    }

    bool running = true;
    while (running) {
        printSep();
        cout << "  Encryption Algorithm RGR — Speck-128 / CBC\n";
        printSep();
        cout << "    1. Текстовый режим (Ввод строк вручную)\n";
        cout << "    2. Файловый режим (Шифрование / дешифрование файлов)\n";
        cout << "    3. Сгенерировать новый ключ и IV\n";
        cout << "    0. Выход\n";
        printSep();
        cout << "    Выбор режима: ";

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

        vector<uint8_t> keyBytes = speck_loadFromFile(KEY_FILE, "КЛЮЧ");
        if (keyBytes.empty()) {
            cout << "\n  [!] Файл ключа не найден: " << KEY_FILE << "\n";
            cout << "  Сначала сгенерируйте ключ (пункт 3).\n";
            continue;
        }

        SpeckHandle handle = mod.create();
        if (handle == nullptr) {
            cout << "\n  [!] Не удалось создать объект Speck в библиотеке.\n";
            continue;
        }

        if (!mod.setKey(handle, keyBytes.data(), keyBytes.size())) {
            cout << "\n  [!] Неверная длина ключа в файле " << KEY_FILE << "\n";
            mod.destroy(handle);
            continue;
        }
        cout << "  [OK] Ключ успешно инициализирован в контексте шифра.\n";

        if (choice == 1) {
            modeText(mod, handle);
        } else {
            modeFile(mod, handle);
        }

        mod.destroy(handle);
    }


}