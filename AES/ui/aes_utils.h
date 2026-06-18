#pragma once

#include "aes_export.h"
#include <string>
#include <vector>

// Генерирует случайный 16-байтный ключ и сохраняет в файл
AES_API bool generate_and_save_key(const std::string& key_path, unsigned char* out_key);

// Загружает ключ из файла
AES_API bool load_key(const std::string& key_path, unsigned char* out_key);

// Генерирует случайный IV (16 байт)
AES_API void generate_iv(unsigned char* iv);

// Сохраняет IV + расширение + зашифрованные данные
// Формат:
// [16 байт IV]
// [N байт расширение]
// [зашифрованные данные]
AES_API bool save_encrypted_file(
    const std::string& path,
    const unsigned char* iv,
    const std::string& extension,
    const std::vector<unsigned char>& ciphertext
);

// Загружает IV + расширение + зашифрованные данные
AES_API bool load_encrypted_file(
    const std::string& path,
    unsigned char* out_iv,
    std::string& out_extension,
    std::vector<unsigned char>& out_ciphertext
);

// Читает любой файл в байтовый вектор
AES_API bool read_binary_file(const std::string& path, std::vector<unsigned char>& out_data);

// Записывает байтовый вектор в файл
AES_API bool write_binary_file(const std::string& path, const std::vector<unsigned char>& data);

// Выводит массив байт в hex-формате
AES_API void print_hex(const std::string& label, const unsigned char* data, int size);