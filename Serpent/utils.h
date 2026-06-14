#pragma once

#include <string>
#include <vector>

// Размер поля для хранения расширения файла внутри зашифрованного файла (в байтах).
// Расширение записывается без точки, дополняется нулевыми байтами справа.
const int EXT_FIELD_SIZE = 8;

// Генерирует случайный 16-байтный ключ и сохраняет в файл
bool generate_and_save_key(const std::string& key_path, unsigned char* out_key);

// Загружает ключ из файла
bool load_key(const std::string& key_path, unsigned char* out_key);

// Генерирует случайный IV (16 байт)
void generate_iv(unsigned char* iv);

// Сохраняет зашифрованный файл в формате:
// [16 байт IV][8 байт расширение исходного файла][зашифрованные данные...]
bool save_encrypted_file(const std::string& path,
                         const unsigned char* iv,
                         const std::string& original_extension,
                         const std::vector<unsigned char>& ciphertext);

// Загружает зашифрованный файл и разбирает его на части:
// IV, расширение исходного файла и сами зашифрованные данные
bool load_encrypted_file(const std::string& path,
                         unsigned char* out_iv,
                         std::string& out_extension,
                         std::vector<unsigned char>& out_ciphertext);

// Читает любой файл целиком в байтовый вектор
bool read_binary_file(const std::string& path, std::vector<unsigned char>& out_data);

// Записывает байтовый вектор в файл
bool write_binary_file(const std::string& path, const std::vector<unsigned char>& data);

// Возвращает расширение файла без точки
std::string get_file_extension(const std::string& filename);

// Возвращает имя файла без расширения
std::string get_file_name_without_extension(const std::string& filename);

// Выводит массив байт в hex-формате
void print_hex(const std::string& label, const unsigned char* data, int size);
