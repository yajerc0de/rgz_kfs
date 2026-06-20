#include "blowfish_module_api.h"
#include "blowfish_utils.h"

#include <iostream>
#include <string>
#include <limits>
#include <algorithm>

using namespace std;

// =============================================================================
//  blowfish_ui.cpp — консольный интерфейс модуля Blowfish
//
//  ВАЖНО: этот файл НЕ знает о структуре BFKey напрямую. Вся работа с
//  шифром идёт через BlowfishModule — struct с указателями на функции,
//  который грузит blowfish.dll/blowfish.so в рантайме
//  (см. blowfish_module_api.h, blowfish_capi.h).
// =============================================================================

static const string KEY_FILE = "blowfish_key.bin";
static const string IV_FILE  = "blowfish_iv.bin";

#ifdef _WIN32
static const string LIB_PATH = "blowfish.dll";
#elif defined(__APPLE__)
static const string LIB_PATH = "./blowfish.dylib";
#else
static const string LIB_PATH = "./blowfish.so";
#endif

static void print_sep(char ch = '-', int w = 50) {
    cout << string(w, ch) << "\n";
}

// =============================================================================
//  input_key — считать ключ от пользователя в HEX-формате
// =============================================================================

static bool input_key(vector<uint8_t>& keyBytes) {
    cout << "\n  Введите ключ в HEX-формате\n";
    cout << "  (от 8 до 112 символов, т.е. 4-56 байт)\n";
    cout << "  Ключ: ";

    string hexKey;
    cin >> hexKey;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    transform(hexKey.begin(), hexKey.end(), hexKey.begin(), ::tolower);

    if (!bf_ui_hex_to_bytes(hexKey, keyBytes)) {
        cout << "\n  [!] Неверный HEX-формат ключа.\n";
        return false;
    }
    if (keyBytes.size() < BLOWFISH_KEY_MIN || keyBytes.size() > BLOWFISH_KEY_MAX) {
        cout << "\n  [!] Длина ключа " << keyBytes.size()
             << " байт. Допустимо: " << BLOWFISH_KEY_MIN << "-"
             << BLOWFISH_KEY_MAX << " байт.\n";
        return false;
    }
    return true;
}

// =============================================================================
//  Режим 1: шифрование / дешифрование текста
// =============================================================================

static void mode_text(BlowfishModule& mod, BlowfishHandle handle) {
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

        vector<uint8_t> iv = bf_ui_generate_and_save(IV_FILE, BLOWFISH_BLOCK_BYTES, "IV");
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
        cout << "  Шифротекст (HEX): " << bf_ui_bytes_to_hex(cipher) << "\n";
        cout << "  IV сохранён в   : " << IV_FILE  << "\n";
        cout << "  Ключ сохранён в : " << KEY_FILE << "\n";
        print_sep();

    } else if (choice == 2) {
        cout << "\n  Введите шифротекст (HEX): ";
        string hexCipher;
        cin >> hexCipher;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        vector<uint8_t> iv = bf_ui_load_from_file(IV_FILE, "IV");
        if (iv.empty() || iv.size() != BLOWFISH_BLOCK_BYTES) {
            cout << "\n  [!] Неверный IV в файле " << IV_FILE << "\n";
            return;
        }

        vector<uint8_t> cipher;
        if (!bf_ui_hex_to_bytes(hexCipher, cipher)) {
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

static void mode_file(BlowfishModule& mod, BlowfishHandle handle) {
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
        if (!bf_ui_read_file(inPath, plain)) return;

        string outPath = bf_ui_build_encrypt_path(inPath);

        vector<uint8_t> iv = bf_ui_generate_and_save(IV_FILE, BLOWFISH_BLOCK_BYTES, "IV");
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
        string originalName = bf_ui_extract_filename(inPath);
        vector<uint8_t> nameHeader = bf_ui_pack_filename_header(originalName);

        vector<uint8_t> output;
        output.insert(output.end(), nameHeader.begin(), nameHeader.end());
        output.insert(output.end(), iv.begin(),         iv.end());
        output.insert(output.end(), cipher.begin(),     cipher.end());

        if (!bf_ui_write_file(outPath, output)) return;

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
        if (!bf_ui_read_file(inPath, raw)) return;

        string originalName;
        size_t headerSize = 0;
        if (!bf_ui_unpack_filename_header(raw, originalName, headerSize)) {
            cout << "\n  [!] Не удалось прочитать имя файла из метаданных.\n";
            cout << "  Файл повреждён или не был зашифрован этой программой.\n";
            return;
        }

        if (raw.size() < headerSize + (size_t)BLOWFISH_BLOCK_BYTES) {
            cout << "\n  [!] Файл слишком мал или повреждён.\n";
            return;
        }

        vector<uint8_t> iv(raw.begin() + headerSize,
                            raw.begin() + headerSize + BLOWFISH_BLOCK_BYTES);
        vector<uint8_t> cipher(raw.begin() + headerSize + BLOWFISH_BLOCK_BYTES,
                                raw.end());

        string outPath = bf_ui_build_decrypt_path(originalName);

        cout << "  [ИМЯ] Восстановлено из метаданных: " << originalName << "\n";
        cout << "  [IV]  Извлечён из файла\n";
        cout << "  [IV]  HEX: " << bf_ui_bytes_to_hex(iv) << "\n";

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

        if (!bf_ui_write_file(outPath, plain)) return;

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
//  У Blowfish длина ключа переменная — пользователь выбирает сам
// =============================================================================

static void mode_key_gen() {
    cout << "\n  Генератор ключей Blowfish\n";
    cout << "  Введите желаемую длину ключа в байтах ("
         << BLOWFISH_KEY_MIN << "-" << BLOWFISH_KEY_MAX << "): ";

    int len = 0;
    cin >> len;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (len < BLOWFISH_KEY_MIN || len > BLOWFISH_KEY_MAX) {
        cout << "\n  [!] Недопустимая длина. Используем 32 байта.\n";
        len = 32;
    }

    vector<uint8_t> key = bf_ui_generate_and_save(KEY_FILE, (size_t)len, "КЛЮЧ");
    if (key.empty()) return;

    cout << "\n";
    print_sep();
    cout << "  Длина ключа : " << len << " байт (" << len * 8 << " бит)\n";
    cout << "  Файл        : " << KEY_FILE << "\n";
    print_sep();
}

// =============================================================================
//  runBlowfish() — точка входа из main.cpp
// =============================================================================

void runBlowfish() {
    cout << "\n";
    print_sep('=');
    cout << "  Blowfish — симметричный блочный шифр\n";
    cout << "  Блок: 64 бит | Ключ: 32-448 бит | Раундов: 16 | Режим: CBC\n";
    print_sep('=');

    BlowfishModule mod;
    if (!blowfish_load(&mod, LIB_PATH)) {
        cout << "\n  [!] Не удалось загрузить библиотеку: " << LIB_PATH << "\n";
        cout << "  Причина: " << blowfish_last_error(&mod) << "\n";
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

        vector<uint8_t> keyBytes = bf_ui_load_from_file(KEY_FILE, "КЛЮЧ");
        if (keyBytes.empty()) {
            cout << "\n  [!] Файл ключа не найден: " << KEY_FILE << "\n";
            cout << "  Сначала сгенерируйте ключ (пункт 3).\n";
            continue;
        }

        BlowfishHandle handle = mod.create();
        if (handle == nullptr) {
            cout << "\n  [!] Не удалось создать контекст Blowfish в библиотеке.\n";
            continue;
        }

        if (!mod.setKey(handle, keyBytes.data(), keyBytes.size())) {
            cout << "\n  [!] Неверная длина ключа в файле " << KEY_FILE << "\n";
            mod.destroy(handle);
            continue;
        }
        cout << "  [OK] Ключ загружен. Key Schedule выполнен.\n";

        if (choice == 1) mode_text(mod, handle);
        else             mode_file(mod, handle);

        mod.destroy(handle);
    }
}