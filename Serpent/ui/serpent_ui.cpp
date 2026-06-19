#include "serpent_ui.h"

#include "serpent_module_api.h"
#include "serpent_utils.h"

#include <iostream>
#include <limits>
#include <vector>
#include <string>

using namespace std;

static SerpentModule g_serpent;

static SerpentHandle g_handle = nullptr;

static SerpentInputSource ask_input_source() {
    cout << "\nВыберите источник данных\n";
    cout << "[1] Текст\n";
    cout << "[2] Файл\n";
    cout << "> ";

    int choice = 0;
    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 1)
        return SerpentInputSource::TEXT_CONSOLE;

    return SerpentInputSource::BINARY_FILE;
}

SerpentAppMode serpent_show_main_menu() {
    cout << "\n=========================\n";
    cout << " Serpent CBC\n";
    cout << "=========================\n";
    cout << "[1] Шифрование\n";
    cout << "[2] Дешифрование\n";
    cout << "[0] Выход\n";
    cout << "> ";

    string choice;
    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == "1") return SerpentAppMode::ENCRYPT;
    if (choice == "2") return SerpentAppMode::DECRYPT;
    if (choice == "0") return SerpentAppMode::EXIT;

    return SerpentAppMode::UNKNOWN;
}

void serpent_run_encrypt_mode() {
    SerpentInputSource src = ask_input_source();

    vector<unsigned char> plaintext;
    string extension;
    string out_filename;

    if (src == SerpentInputSource::TEXT_CONSOLE) {
        string text;
        cout << "Введите текст:\n";
        getline(cin, text);

        plaintext.assign(text.begin(), text.end());
        extension = "txt";
        out_filename = "encrypted_text.bin";
    }
    else
    {
        string filepath;
        cout << "Путь к файлу: ";
        getline(cin, filepath);

        if (!serpent_read_binary_file(filepath, plaintext))
            return;

        extension = serpent_get_extension(filepath);
        out_filename = serpent_strip_extension(filepath) + "_encrypted.bin";
    }

    unsigned char key[SERPENT_BLOCK_BYTES];
    string key_path = "./key.bin";

    if (!serpent_generate_and_save_key(key_path, key))
        return;

    if (!g_serpent.setKey(g_handle, key, SERPENT_KEY_BYTES)) {
        cout << "Ошибка установки ключа\n";
        return;
    }

    unsigned char iv[SERPENT_BLOCK_BYTES];
    serpent_generate_iv(iv);

    uint8_t* cipherData = nullptr;
    size_t cipherLen = 0;

    if (!g_serpent.encryptCbc(g_handle, plaintext.data(), plaintext.size(), iv, &cipherData, &cipherLen)) {
        cout << "Ошибка шифрования\n";
        return;
    }

    vector<unsigned char> ciphertext(cipherData, cipherData + cipherLen);
    g_serpent.freeBuffer(cipherData);

    serpent_save_encrypted_file(out_filename, iv, extension, ciphertext);
}

void serpent_run_decrypt_mode() {
    string enc_path;
    string key_path;

    cout << "Зашифрованный файл: ";
    getline(cin, enc_path);

    cout << "Файл ключа: ";
    getline(cin, key_path);

    unsigned char key[SERPENT_KEY_BYTES];

    if (!serpent_load_key(key_path, key))
        return;

    if (!g_serpent.setKey(g_handle, key, SERPENT_KEY_BYTES))
        return;

    unsigned char iv[SERPENT_BLOCK_BYTES];
    vector<unsigned char> ciphertext;
    string extension;

    if (!serpent_load_encrypted_file(enc_path, iv, extension, ciphertext))
        return;

    uint8_t* plainData = nullptr;
    size_t plainLen = 0;

    if (!g_serpent.decryptCbc(g_handle, ciphertext.data(), ciphertext.size(), iv, &plainData, &plainLen)) {
        cout << "Ошибка расшифровки\n";
        return;
    }

    vector<unsigned char> plaintext(plainData, plainData + plainLen);
    g_serpent.freeBuffer(plainData);

    string out_name = serpent_strip_extension(enc_path) + "_decrypted";
    if (!extension.empty())
        out_name += "." + extension;

    serpent_write_binary_file(out_name, plaintext);
}

void serpent_run_ui()
{
#ifdef _WIN32
    const string libName = "serpent.dll";
#elif defined(__APPLE__)
    const string libName = "./libserpent.dylib";
#else
    const string libName = "./serpent.so";
#endif

    if (!serpent_load(&g_serpent, libName))
    {
        cout
            << "Ошибка загрузки Serpent: "
            << serpent_last_error(&g_serpent)
            << "\n";
        return;
    }

    g_handle = g_serpent.create();

    if (g_handle == nullptr)
        return;

    while (true)
    {
        SerpentAppMode mode = serpent_show_main_menu();

        if (mode == SerpentAppMode::EXIT)
            break;

        if (mode == SerpentAppMode::ENCRYPT)
            serpent_run_encrypt_mode();

        if (mode == SerpentAppMode::DECRYPT)
            serpent_run_decrypt_mode();
    }

    g_serpent.destroy(g_handle);
}
