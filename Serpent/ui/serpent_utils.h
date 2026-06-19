#pragma once

#include <string>
#include <vector>

// Генерирует случайный 16-байтный ключ и сохраняет в файл
bool serpent_generate_and_save_key(const std::string& key_path, unsigned char* out_key);

// Загружает ключ из файла
bool serpent_load_key(const std::string& key_path, unsigned char* out_key);

// Генерирует случайный IV (16 байт)
void serpent_generate_iv(unsigned char* iv);

// Сохраняет IV + расширение + зашифрованные данные
// Формат:
// [16 байт IV]
// [1 байт — длина расширения N]
// [N байт — расширение без точки]
// [зашифрованные данные]
bool serpent_save_encrypted_file(
    const std::string& path,
    const unsigned char* iv,
    const std::string& extension,
    const std::vector<unsigned char>& ciphertext
);

// Загружает IV + расширение + зашифрованные данные
bool serpent_load_encrypted_file(
    const std::string& path,
    unsigned char* out_iv,
    std::string& out_extension,
    std::vector<unsigned char>& out_ciphertext
);

// Читает любой файл в байтовый вектор
bool serpent_read_binary_file(const std::string& path, std::vector<unsigned char>& out_data);

// Записывает байтовый вектор в файл
bool serpent_write_binary_file(const std::string& path, const std::vector<unsigned char>& data);

// Выводит массив байт в hex-формате
void serpent_print_hex(const std::string& label, const unsigned char* data, int size);

// Возвращает расширение файла без точки
std::string serpent_get_extension(const std::string& filename);

// Возвращает имя файла без расширения
std::string serpent_strip_extension(const std::string& filename);
