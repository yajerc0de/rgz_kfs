#include "utils.h"
#include "serpent_core.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <random>

bool generate_and_save_key(const std::string& key_path, unsigned char* out_key) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);

    for (int i = 0; i < KEY_SIZE; ++i)
        out_key[i] = (unsigned char)dis(gen);

    std::ofstream f(key_path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось создать файл ключа: " << key_path << "\n";
        return false;
    }
    f.write(reinterpret_cast<const char*>(out_key), KEY_SIZE);
    f.close();

    print_hex("Сгенерирован ключ (HEX)", out_key, KEY_SIZE);
    return true;
}

bool load_key(const std::string& key_path, unsigned char* out_key) {
    std::ifstream f(key_path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось открыть файл ключа: " << key_path << "\n";
        return false;
    }
    f.read(reinterpret_cast<char*>(out_key), KEY_SIZE);
    if (f.gcount() != KEY_SIZE) {
        std::cerr << "[Ошибка] Файл ключа повреждён или неполный\n";
        return false;
    }
    f.close();
    return true;
}

void generate_iv(unsigned char* iv) {
    std::random_device rd;
    for (int i = 0; i < BLOCK_SIZE; ++i)
        iv[i] = (unsigned char)(rd() % 256);
}

bool save_encrypted_file(const std::string& path,
                         const unsigned char* iv,
                         const std::string& original_extension,
                         const std::vector<unsigned char>& ciphertext)
{
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось создать файл: " << path << "\n";
        return false;
    }

    f.write(reinterpret_cast<const char*>(iv), BLOCK_SIZE);

    unsigned char ext_field[EXT_FIELD_SIZE];
    for (int i = 0; i < EXT_FIELD_SIZE; ++i) ext_field[i] = 0;

    int ext_len = (int)original_extension.size();
    if (ext_len > EXT_FIELD_SIZE) ext_len = EXT_FIELD_SIZE;

    for (int i = 0; i < ext_len; ++i)
        ext_field[i] = (unsigned char)original_extension[i];

    f.write(reinterpret_cast<const char*>(ext_field), EXT_FIELD_SIZE);

    // 3) Записываем сами зашифрованные данные
    f.write(reinterpret_cast<const char*>(ciphertext.data()), ciphertext.size());

    f.close();
    return true;
}

bool load_encrypted_file(const std::string& path,
                         unsigned char* out_iv,
                         std::string& out_extension,
                         std::vector<unsigned char>& out_ciphertext)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[Ошибка] Не удалось открыть файл: " << path << "\n";
        return false;
    }

    f.read(reinterpret_cast<char*>(out_iv), BLOCK_SIZE);
    if (f.gcount() != BLOCK_SIZE) {
        std::cerr << "[Ошибка] Файл слишком маленький (нет IV)\n";
        return false;
    }

    unsigned char ext_field[EXT_FIELD_SIZE];
    f.read(reinterpret_cast<char*>(ext_field), EXT_FIELD_SIZE);
    if (f.gcount() != EXT_FIELD_SIZE) {
        std::cerr << "[Ошибка] Файл слишком маленький (нет поля расширения)\n";
        return false;
    }

    out_extension = "";
    for (int i = 0; i < EXT_FIELD_SIZE; ++i) {
        if (ext_field[i] == 0) break;
        out_extension += (char)ext_field[i];
    }

    // 3) Читаем оставшиеся данные - это зашифрованный текст
    out_ciphertext.assign(std::istreambuf_iterator<char>(f),
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

std::string get_file_extension(const std::string& filename) {
    // Ищем последнюю точку после последнего слэша
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

std::string get_file_name_without_extension(const std::string& filename) {
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

void print_hex(const std::string& label, const unsigned char* data, int size) {
    std::cout << label << ": ";
    for (int i = 0; i < size; ++i)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    std::cout << std::dec << "\n";
}
