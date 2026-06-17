#pragma once

#include <string>
#include <vector>
#include <serpent_export.h>

// Размер поля для хранения расширения файла внутри зашифрованного файла (в байтах).
// Расширение записывается без точки, дополняется нулевыми байтами справа.
const int EXT_FIELD_SIZE = 8;

// Генерирует случайный 16-байтный ключ и сохраняет в файл
SERPENT_API bool generate_and_save_key(const std::string& key_path, unsigned char* out_key);

// Загружает ключ из файла
SERPENT_API bool load_key(const std::string& key_path, unsigned char* out_key);

// Генерирует случайный IV (16 байт)
SERPENT_API void generate_iv(unsigned char* iv);

// Сохраняет зашифрованный файл в формате:
// [16 байт IV][8 байт расширение исходного файла][зашифрованные данные...]
SERPENT_API bool save_encrypted_file(const std::string& path,
                         const unsigned char* iv,
                         const std::string& original_extension,
                         const std::vector<unsigned char>& ciphertext);

// Загружает зашифрованный файл и разбирает его на части:
// IV, расширение исходного файла и сами зашифрованные данные
SERPENT_API bool load_encrypted_file(const std::string& path,
                         unsigned char* out_iv,
                         std::string& out_extension,
                         std::vector<unsigned char>& out_ciphertext);

// Читает любой файл целиком в байтовый вектор
SERPENT_API bool read_binary_file(const std::string& path, std::vector<unsigned char>& out_data);

// Записывает байтовый вектор в файл
SERPENT_API bool write_binary_file(const std::string& path, const std::vector<unsigned char>& data);

// Возвращает расширение файла без точки
SERPENT_API std::string get_file_extension(const std::string& filename);

// Возвращает имя файла без расширения
SERPENT_API std::string get_file_name_without_extension(const std::string& filename);

// Выводит массив байт в hex-формате
SERPENT_API void print_hex(const std::string& label, const unsigned char* data, int size);
