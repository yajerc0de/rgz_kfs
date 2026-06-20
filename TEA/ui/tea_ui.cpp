#include "tea_module_api.h"
#include "tea_utils.h"

#include <iostream>
#include <string>
#include <limits>

using namespace std;

// =============================================================================
//  tea_ui.cpp — консольный интерфейс модуля TEA
//
//  ВАЖНО: этот файл НЕ знает о структуре TeaKey напрямую. Вся работа с
//  шифром идёт через TEAModule — struct с указателями на функции, который
//  грузит tea.dll/tea.so в рантайме (см. tea_module_api.h, tea_capi.h).
// =============================================================================

static const string KEY_FILE = "tea_key.bin";
static const string IV_FILE  = "tea_iv.bin";

#ifdef _WIN32
static const string LIB_PATH = "tea.dll";
#elif defined(__APPLE__)
static const string LIB_PATH = "./tea.dylib";
#else
static const string LIB_PATH = "./tea.so";
#endif

static void print_sep(char ch = '-', int w = 50) {
    cout << string(w, ch) << "\n";
}

// =============================================================================
//  Режим 1: шифрование / дешифрование текста
// =============================================================================

static void mode_text(TEAModule& mod, TEAHandle handle) {
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

        vector<uint8_t> iv = tea_generate_and_save(IV_FILE, TEA_CAPI_BLOCK_BYTES, "IV");
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
        mod.freeBuffer(outData);

        cout << "\n";
        print_sep();
        cout << "  Шифротекст (HEX): " << tea_bytes_to_hex(cipher) << "\n";
        cout << "  IV сохранён в   : " << IV_FILE  << "\n";
        cout << "  Ключ сохранён в : " << KEY_FILE << "\n";
        print_sep();

    } else if (choice == 2) {
        cout << "\n  Введите шифротекст (HEX): ";
        string hexCipher;
        cin >> hexCipher;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        vector<uint8_t> iv = tea_load_from_file(IV_FILE, "IV");
        if (iv.empty() || iv.size() != TEA_CAPI_BLOCK_BYTES) {
            cout << "\n  [!] Неверный IV в файле " << IV_FILE << "\n";
            return;
        }

        vector<uint8_t> cipher;
        if (!tea_hex_to_bytes(hexCipher, cipher)) {
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
        print_sep();
        cout << "  Результат: " << string(plain.begin(), plain.end()) << "\n";
        print_sep();

    } else {
        cout << "\n  [!] Неверный выбор.\n";
    }
}

// =============================================================================
//  Режим 2: шифрование / дешифрование файла
// =============================================================================

static void mode_file(TEAModule& mod, TEAHandle handle) {
    cout << "\n  Файловый режим:\n";
    cout << "    1. Зашифровать файл\n";
    cout << "    2. Расшифровать файл\n";
    cout << "  Выбор: ";

    int choice = 0;
    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 1) {
        // ── Шифрование ────────────────────────────────────────────────────────
        cout << "\n  Путь к исходному файлу: ";
        string inPath;
        getline(cin, inPath);

        vector<uint8_t> plain;
        if (!tea_read_file(inPath, plain)) return;

        string outPath = tea_build_encrypt_path(inPath);

        vector<uint8_t> iv = tea_generate_and_save(IV_FILE, TEA_CAPI_BLOCK_BYTES, "IV");
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

        // Формат файла: [заголовок имени][8 байт IV][шифротекст]
        string originalName = tea_extract_filename(inPath);
        vector<uint8_t> nameHeader = tea_pack_filename_header(originalName);

        vector<uint8_t> output;
        output.insert(output.end(), nameHeader.begin(), nameHeader.end());
        output.insert(output.end(), iv.begin(),         iv.end());
        output.insert(output.end(), cipher.begin(),     cipher.end());

        if (!tea_write_file(outPath, output)) return;

        cout << "\n";
        print_sep();
        cout << "  Исходный файл     : " << inPath        << "\n";
        cout << "  Оригинальное имя  : " << originalName  << " (сохранено внутри .bin)\n";
        cout << "  Зашифрован в      : " << outPath       << "\n";
        cout << "  Исходный размер   : " << plain.size()  << " байт\n";
        cout << "  Итоговый размер   : " << output.size() << " байт\n";
        cout << "  Ключ сохранён в   : " << KEY_FILE      << "\n";
        cout << "  IV сохранён в     : " << IV_FILE       << "\n";
        print_sep();

    } else if (choice == 2) {
        // ── Дешифрование ──────────────────────────────────────────────────────
        cout << "\n  Путь к зашифрованному файлу: ";
        string inPath;
        getline(cin, inPath);

        vector<uint8_t> raw;
        if (!tea_read_file(inPath, raw)) return;

        string originalName;
        size_t headerSize = 0;
        if (!tea_unpack_filename_header(raw, originalName, headerSize)) {
            cout << "\n  [!] Не удалось прочитать имя файла из метаданных.\n";
            cout << "  Файл повреждён или не был зашифрован этой программой.\n";
            return;
        }

        if (raw.size() < headerSize + (size_t)TEA_CAPI_BLOCK_BYTES) {
            cout << "\n  [!] Файл слишком мал или повреждён.\n";
            return;
        }

        vector<uint8_t> iv(raw.begin() + headerSize,
                            raw.begin() + headerSize + TEA_CAPI_BLOCK_BYTES);
        vector<uint8_t> cipher(raw.begin() + headerSize + TEA_CAPI_BLOCK_BYTES,
                                raw.end());

        string outPath = tea_build_decrypt_path(originalName);

        cout << "  [ИМЯ] Восстановлено из метаданных: " << originalName << "\n";
        cout << "  [IV]  Извлечён из файла\n";
        cout << "  [IV]  HEX: " << tea_bytes_to_hex(iv) << "\n";

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

        if (!tea_write_file(outPath, plain)) return;

        cout << "\n";
        print_sep();
        cout << "  Зашифрованный файл: " << inPath      << "\n";
        cout << "  Расшифрован в     : " << outPath     << "\n";
        cout << "  Размер данных     : " << plain.size() << " байт\n";
        print_sep();

    } else {
        cout << "\n  [!] Неверный выбор.\n";
    }
}

// =============================================================================
//  Режим 3: генератор ключей
// =============================================================================

static void mode_key_gen() {
    cout << "\n  Генератор ключей TEA\n";
    cout << "  Длина ключа фиксирована: "
         << TEA_CAPI_KEY_BYTES << " байт ("
         << TEA_CAPI_KEY_BYTES * 8 << " бит)\n\n";

    vector<uint8_t> key = tea_generate_and_save(KEY_FILE, TEA_CAPI_KEY_BYTES, "КЛЮЧ");
    if (key.empty()) return;

    cout << "\n";
    print_sep();
    cout << "  Файл : " << KEY_FILE << "\n";
    cout << "  Длина: " << TEA_CAPI_KEY_BYTES << " байт ("
         << TEA_CAPI_KEY_BYTES * 8 << " бит)\n";
    print_sep();
}

// =============================================================================
//  runTEA() — точка входа из main.cpp
// =============================================================================

void runTEA() {
    cout << "\n";
    print_sep('=');
    cout << "  TEA — Tiny Encryption Algorithm\n";
    cout << "  Блок: 64 бит | Ключ: 128 бит (фикс.) | Раундов: 64 | Режим: CBC\n";
    print_sep('=');

    TEAModule mod;
    if (!tea_load(&mod, LIB_PATH)) {
        cout << "\n  [!] Не удалось загрузить библиотеку: " << LIB_PATH << "\n";
        cout << "  Причина: " << tea_last_error(&mod) << "\n";
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
        print_sep();
        cout << "  Выбор: ";

        int choice = 0;
        cin >> choice;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (choice == 0) {
            running = false;
            continue;
        }

        if (choice == 3) {
            mode_key_gen();
            continue;
        }

        if (choice != 1 && choice != 2) {
            cout << "\n  [!] Неверный выбор.\n";
            continue;
        }

        vector<uint8_t> keyBytes = tea_load_from_file(KEY_FILE, "КЛЮЧ");
        if (keyBytes.empty()) {
            cout << "\n  [!] Файл ключа не найден: " << KEY_FILE << "\n";
            cout << "  Сначала сгенерируйте ключ (пункт 3).\n";
            continue;
        }

        TEAHandle handle = mod.create();
        if (handle == nullptr) {
            cout << "\n  [!] Не удалось создать контекст TEA в библиотеке.\n";
            continue;
        }

        if (!mod.setKey(handle, keyBytes.data(), keyBytes.size())) {
            cout << "\n  [!] Неверная длина ключа в файле " << KEY_FILE << "\n";
            cout << "  TEA требует ровно " << TEA_CAPI_KEY_BYTES << " байт.\n";
            mod.destroy(handle);
            continue;
        }
        cout << "  [OK] Ключ загружен.\n";

        if (choice == 1) mode_text(mod, handle);
        else             mode_file(mod, handle);

        mod.destroy(handle);
    }
}