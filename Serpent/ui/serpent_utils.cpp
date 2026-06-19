#include "serpent_utils.h"
#include "../capi/serpent_capi.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>

bool serpent_generate_and_save_key(const std::string& key_path, unsigned char* out_key) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);

    for (int i = 0; i < SERPENT_KEY_BYTES; ++i)
        out_key[i] = (unsigned char)dis(gen);

    std::ofstream f(key_path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось создать файл ключа: " << key_path << "\n";
        return false;
    }
    f.write(reinterpret_cast<const char*>(out_key), SERPENT_KEY_BYTES);
    f.close();

    serpent_print_hex("Сгенерирован ключ (HEX)", out_key, SERPENT_KEY_BYTES);
    return true;
}

bool serpent_load_key(const std::string& key_path, unsigned char* out_key) {
    std::ifstream f(key_path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось открыть файл ключа: " << key_path << "\n";
        return false;
    }
    f.read(reinterpret_cast<char*>(out_key), SERPENT_KEY_BYTES);
    if (f.gcount() != SERPENT_KEY_BYTES) {
        std::cerr << "[Ошибка] Файл ключа повреждён или неполный\n";
        return false;
    }
    f.close();
    return true;
}

void serpent_generate_iv(unsigned char* iv) {
    std::random_device rd;
    for (int i = 0; i < SERPENT_BLOCK_BYTES; ++i)
        iv[i] = (unsigned char)(rd() % 256);
}

bool serpent_save_encrypted_file(
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

    f.write(reinterpret_cast<const char*>(iv), SERPENT_BLOCK_BYTES);

    unsigned char ext_len = (unsigned char)extension.size();
    f.write(reinterpret_cast<const char*>(&ext_len), 1);

    if (ext_len > 0)
        f.write(extension.c_str(), ext_len);

    f.write(reinterpret_cast<const char*>(ciphertext.data()), ciphertext.size());

    f.close();
    return true;
}

bool serpent_load_encrypted_file(
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

    f.read(reinterpret_cast<char*>(out_iv), SERPENT_BLOCK_BYTES);
    if (f.gcount() != SERPENT_BLOCK_BYTES) {
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

bool serpent_read_binary_file(const std::string& path, std::vector<unsigned char>& out_data) {
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

bool serpent_write_binary_file(const std::string& path, const std::vector<unsigned char>& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось создать файл: " << path << "\n";
        return false;
    }
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
    f.close();
    return true;
}

void serpent_print_hex(const std::string& label, const unsigned char* data, int size) {
    std::cout << label << ": ";
    for (int i = 0; i < size; ++i)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    std::cout << std::dec << "\n";
}

std::string serpent_get_extension(const std::string& filename) {
    int slash_pos = -1;
    for (int i = (int)filename.size() - 1; i >= 0; --i) {
        if (filename[i] == '/' || filename[i] == '\\') {
            slash_pos = i;
            break;
        }
    }
    int dot_pos = -1;
    for (int i = (int)filename.size() - 1; i > slash_pos; --i) {
        if (filename[i] == '.') {
            dot_pos = i;
            break;
        }
    }
    if (dot_pos <= slash_pos + 1 || dot_pos == -1) return "";
    return filename.substr(dot_pos + 1);
}

std::string serpent_strip_extension(const std::string& filename) {
    int slash_pos = -1;
    for (int i = (int)filename.size() - 1; i >= 0; --i) {
        if (filename[i] == '/' || filename[i] == '\\') {
            slash_pos = i;
            break;
        }
    }
    std::string name = filename.substr(slash_pos + 1);
    int dot_pos = -1;
    for (int i = (int)name.size() - 1; i > 0; --i) {
        if (name[i] == '.') {
            dot_pos = i;
            break;
        }
    }
    if (dot_pos > 0) return name.substr(0, dot_pos);
    return name;
}
