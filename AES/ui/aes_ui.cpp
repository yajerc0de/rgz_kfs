#include "aes_ui.h"

#include "aes_module_api.h"
#include "aes_utils.h"

#include <iostream>
#include <limits>
#include <vector>
#include <string>

using namespace std;

static AESModule g_aes;

static AESHandle g_handle = nullptr;

static string get_output_dir() {
    return "./";
}

static string strip_extension(const string& filename) {
    int slash_pos = -1;

    for (int i = (int)filename.size() - 1; i >= 0; --i) {
        if (filename[i] == '/' || filename[i] == '\\') {
            slash_pos = i;
            break;
        }
    }

    string name = filename.substr(slash_pos + 1);

    int dot_pos = -1;

    for (int i = (int)name.size() - 1; i >= 0; --i) {
        if (name[i] == '.') {
            dot_pos = i;
            break;
        }
    }

    if (dot_pos > 0) return name.substr(0, dot_pos);
    return name;


}

static string get_extension(const string& filename) {
    int slash_pos = -1;

    for (int i = (int)filename.size() - 1; i >= 0; --i) {
        if (filename[i] == '/' || filename[i] == '\\') {
            slash_pos = i;
            break;
        }
    }

    string name = filename.substr(slash_pos + 1);

    int dot_pos = -1;

    for (int i = (int)name.size() - 1; i >= 0; --i) {
        if (name[i] == '.') {
            dot_pos = i;
            break;
        }
    }
    if (dot_pos >= 0) return name.substr(dot_pos + 1);

    return "";
}

static InputSource ask_input_source() {
    cout << "\nВыберите источник данных\n";
    cout << "[1] Текст\n";
    cout << "[2] Файл\n";
    cout << "> ";

    int choice = 0;

    cin >> choice;

    cin.ignore(numeric_limits<streamsize>::max(),'\n');

    if (choice == 1)
        return InputSource::TEXT_CONSOLE;

    return InputSource::BINARY_FILE;

}

AppMode aes_show_main_menu() {
    cout << "\n=========================\n";
    cout << " AES-128 CBC\n";
    cout << "=========================\n";
    cout << "[1] Шифрование\n";
    cout << "[2] Дешифрование\n";
    cout << "[0] Выход\n";
    cout << "> ";

    string choice;

    cin >> choice;

    cin.ignore(numeric_limits<streamsize>::max(),'\n');

    if (choice == "1") return AppMode::ENCRYPT;

    if (choice == "2") return AppMode::DECRYPT;

    if (choice == "0") return AppMode::EXIT;

    return AppMode::UNKNOWN;
}

void aes_run_encrypt_mode() {
    InputSource src = ask_input_source();

    vector<unsigned char> plaintext;

    string extension;
    string out_filename;

    if (src == InputSource::TEXT_CONSOLE) {
        string text;

        cout << "Введите текст:\n";

        getline(cin, text);

        plaintext.assign(text.begin(),text.end());

        extension = "txt";

        out_filename = "encrypted_text.bin";
    }
    else
    {
        string filepath;

        cout << "Путь к файлу: ";

        getline(cin, filepath);

        if (!read_binary_file(filepath,plaintext))
        {
            return;
        }

        extension = get_extension(filepath);

        out_filename = strip_extension(filepath) + "_encrypted.bin";
    }

    unsigned char key[AES_BLOCK_BYTES];

    string key_path = get_output_dir() + "key.bin";

    if (!generate_and_save_key(key_path, key)) {
        return;
    }

    if (!g_aes.setKey(g_handle, key, AES_BLOCK_BYTES)) {
        cout << "Ошибка установки ключа\n";
        return;
    }

    unsigned char iv[AES_BLOCK_BYTES];

    generate_iv(iv);

    uint8_t* cipherData = nullptr;
    size_t cipherLen = 0;

    if (!g_aes.encryptCbc(g_handle, plaintext.data(), plaintext.size(), iv, &cipherData, &cipherLen)) {
        cout << "Ошибка шифрования\n";
        return;
    }

    vector<unsigned char> ciphertext(
        cipherData,
        cipherData + cipherLen);

    g_aes.freeBuffer(
        cipherData);

    save_encrypted_file(
        out_filename,
        iv,
        extension,
        ciphertext);

}

void aes_run_decrypt_mode() {
    string enc_path;
    string key_path;

    cout << "Зашифрованный файл: ";
    getline(cin, enc_path);

    cout << "Файл ключа: ";
    getline(cin, key_path);

    unsigned char key[AES_BLOCK_BYTES];

    if (!load_key(
            key_path,
            key))
    {
        return;
    }

    if (!g_aes.setKey(
            g_handle,
            key,
            AES_BLOCK_BYTES))
    {
        return;
    }

    unsigned char iv[AES_BLOCK_BYTES];

    vector<unsigned char> ciphertext;

    string extension;

    if (!load_encrypted_file(
            enc_path,
            iv,
            extension,
            ciphertext))
    {
        return;
    }

    uint8_t* plainData = nullptr;
    size_t plainLen = 0;

    if (!g_aes.decryptCbc(
            g_handle,
            ciphertext.data(),
            ciphertext.size(),
            iv,
            &plainData,
            &plainLen))
    {
        cout << "Ошибка расшифровки\n";
        return;
    }

    vector<unsigned char> plaintext(
        plainData,
        plainData + plainLen);

    g_aes.freeBuffer(
        plainData);

    string out_name =
        strip_extension(enc_path)
        + "_decrypted";

    if (!extension.empty())
        out_name += "." + extension;

    write_binary_file(
        out_name,
        plaintext);
}

void aes_run_ui()
{
    #ifdef _WIN32
    const string libName = "aes.dll";
    #elif defined(__APPLE__)
    const string libName = "./aes.dylib";
    #else
    const string libName = "./aes.so";
    #endif

    if (!aes_load(
            &g_aes,
            libName))
    {
        cout
            << "Ошибка загрузки AES: "
            << aes_last_error(&g_aes)
            << "\n";

        return;
    }

    g_handle =
        g_aes.create();

    if (g_handle == nullptr)
    {
        return;
    }

    while (true)
    {
        AppMode mode =
            aes_show_main_menu();

        if (mode == AppMode::EXIT)
            break;

        if (mode == AppMode::ENCRYPT)
            aes_run_encrypt_mode();

        if (mode == AppMode::DECRYPT)
            aes_run_decrypt_mode();
    }

    g_aes.destroy(
        g_handle);

}