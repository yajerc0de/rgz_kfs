#include "aes_utils.h"
#include "../capi/aes_capi.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>

bool generate_and_save_key(const std::string& key_path, unsigned char* out_key) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);

    for (int i = 0; i < AES_BLOCK_BYTES; ++i)
        out_key[i] = (unsigned char)dis(gen);

    std::ofstream f(key_path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось создать файл ключа: " << key_path << "\n";
        return false;
    }
    f.write(reinterpret_cast<const char*>(out_key), AES_BLOCK_BYTES);
    f.close();

    print_hex("Сгенерирован ключ (HEX)", out_key, AES_BLOCK_BYTES);
    return true;
}

bool load_key(const std::string& key_path, unsigned char* out_key) {
    std::ifstream f(key_path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось открыть файл ключа: " << key_path << "\n";
        return false;
    }
    f.read(reinterpret_cast<char*>(out_key), AES_BLOCK_BYTES);
    if (f.gcount() != AES_BLOCK_BYTES) {
        std::cerr << "[Ошибка] Файл ключа повреждён или неполный\n";
        return false;
    }
    f.close();
    return true;
}

void generate_iv(unsigned char* iv) {
    std::random_device rd;
    for (int i = 0; i < AES_BLOCK_BYTES; ++i)
        iv[i] = (unsigned char)(rd() % 256);
}

bool save_encrypted_file(
    const std::string& path,
    const unsigned char* iv,
    const std::string& extension,
    const std::vector<unsigned char>& ciphertext)
{
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось создать файл: " << path << "\n";
        return false;
    }

    f.write(reinterpret_cast<const char*>(iv), AES_BLOCK_BYTES);

    unsigned char ext_len = (unsigned char)extension.size();
    f.write(reinterpret_cast<const char*>(&ext_len), 1);

    if (ext_len > 0)
        f.write(extension.c_str(), ext_len);

    f.write(reinterpret_cast<const char*>(ciphertext.data()),
            ciphertext.size());

    f.close();
    return true;
}

bool load_encrypted_file(
    const std::string& path,
    unsigned char* out_iv,
    std::string& out_extension,
    std::vector<unsigned char>& out_ciphertext)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось открыть файл: " << path << "\n";
        return false;
    }

    f.read(reinterpret_cast<char*>(out_iv), AES_BLOCK_BYTES);
    if (f.gcount() != AES_BLOCK_BYTES) {
        std::cerr << "[Ошибка] Файл слишком маленький (нет IV)\n";
        return false;
    }

    unsigned char ext_len = 0;
    f.read(reinterpret_cast<char*>(&ext_len), 1);

    out_extension.clear();
    if (ext_len > 0) {
        out_extension.resize(ext_len);
        f.read(&out_extension[0], ext_len);
    }

    out_ciphertext.assign(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>());

    f.close();

    if (out_ciphertext.empty()) {
        std::cerr << "[Ошибка] Нет зашифрованных данных в файле\n";
        return false;
    }

    return true;
}

bool read_binary_file(const std::string& path, std::vector<unsigned char>& out_data) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось открыть файл: " << path << "\n";
        return false;
    }

    out_data.assign(std::istreambuf_iterator<char>(f),
                    std::istreambuf_iterator<char>());

    f.close();

    if (out_data.empty()) {
        std::cerr << "[Ошибка] Файл пустой: " << path << "\n";
        return false;
    }

    return true;
}

bool write_binary_file(const std::string& path, const std::vector<unsigned char>& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось создать файл: " << path << "\n";
        return false;
    }

    f.write(reinterpret_cast<const char*>(data.data()), data.size());
    f.close();

    return true;
}

void print_hex(const std::string& label, const unsigned char* data, int size) {
    std::cout << label << ": ";
    for (int i = 0; i < size; ++i)
        std::cout << std::hex
                  << std::setw(2)
                  << std::setfill('0')
                  << (int)data[i];

    std::cout << std::dec << "\n";
}