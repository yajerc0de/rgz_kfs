#pragma once

#include <string>
#include <vector>
#include <cstdint>

// =============================================================================
//  blowfish_utils.h — утилиты для модуля Blowfish
<<<<<<< HEAD
//  Конвертация HEX, работа с файлами, генерация случайных байт
//  Не использует <filesystem> — совместимо с MinGW и старыми компиляторами
//
//  Всё обёрнуто в namespace BlowfishUtils, потому что app.exe линкует
//  utils-файлы ВСЕХ шифров одновременно (TEA, Blowfish, ...), а у них
//  совпадают имена функций (hexToBytes, readFile и т.д.) — без namespace
//  это конфликт линковки (LNK2005 / multiple definition).
=======
//  Конвертация HEX, работа с файлами, генерация случайных байт.
//
//  Все функции имеют префикс bf_ui_ — это заменяет namespace и исключает
//  конфликты линковки при одновременном подключении нескольких шифров
//  (TEA, Blowfish и др.) в одном .exe.
//  Префикс bf_ui_ выбран чтобы не конфликтовать с algo-функциями (bf_*)
//  из blowfish.h.
>>>>>>> BlowTEA
// =============================================================================

namespace BlowfishUtils {

// ─── HEX ─────────────────────────────────────────────────────────────────────

// Конвертировать hex-строку ("a1b2c3...") в вектор байт.
// Возвращает false если строка содержит недопустимые символы или нечётная длина.
bool bf_ui_hex_to_bytes(const std::string& hex, std::vector<uint8_t>& out);

// Конвертировать вектор байт в hex-строку нижнего регистра.
std::string bf_ui_bytes_to_hex(const std::vector<uint8_t>& data);

// ─── Случайные байты ─────────────────────────────────────────────────────────

// Сгенерировать count случайных байт через mt19937 + random_device.
std::vector<uint8_t> bf_ui_random_bytes(size_t count);

// ─── Файлы ───────────────────────────────────────────────────────────────────

// Считать содержимое файла в бинарном режиме.
// При ошибке выводит сообщение в cout и возвращает false.
bool bf_ui_read_file(const std::string& path, std::vector<uint8_t>& data);

// Записать байты в файл в бинарном режиме.
// При ошибке выводит сообщение в cout и возвращает false.
bool bf_ui_write_file(const std::string& path, const std::vector<uint8_t>& data);

// Создать папку если она не существует.
// Возвращает false при ошибке создания.
bool bf_ui_ensure_dir(const std::string& dirPath);

// Извлечь имя файла без пути (например "docs/photo.jpg" -> "photo.jpg").
std::string bf_ui_extract_filename(const std::string& path);

// Извлечь расширение с точкой (например "photo.jpg" -> ".jpg").
std::string bf_ui_extract_extension(const std::string& path);

// Собрать путь для зашифрованного файла.
// Результат: Encryptfiles/encrypted_<имя файла без расширения>.bin
std::string bf_ui_build_encrypt_path(const std::string& sourcePath);

// Собрать путь для расшифрованного файла.
// originalName — оригинальное имя, извлечённое из метаданных внутри .bin файла.
// Результат: Decryptfiles/decrypted_<originalName>
std::string bf_ui_build_decrypt_path(const std::string& originalName);

// ─── Метаданные имени файла ───────────────────────────────────────────────────

// Упаковать оригинальное имя файла в бинарный префикс:
// [4 байта: длина имени (little-endian uint32_t)][N байт: имя файла UTF-8]
std::vector<uint8_t> bf_ui_pack_filename_header(const std::string& originalFilename);

// Распаковать имя файла из начала буфера.
// Возвращает true при успехе; originalFilename получает извлечённое имя,
// bytesConsumed — сколько байт занял заголовок (4 + длина имени).
bool bf_ui_unpack_filename_header(const std::vector<uint8_t>& data,
                                   std::string& originalFilename,
                                   size_t& bytesConsumed);

// ─── Ключи и IV ──────────────────────────────────────────────────────────────

<<<<<<< HEAD
vector<uint8_t> generateAndSave(const string& filepath, size_t byteCount, const string& label);
vector<uint8_t> loadFromFile(const string& filepath, const string& label);

} // namespace BlowfishUtils
=======
// Сгенерировать случайные байты, сохранить в файл и вывести HEX в консоль.
// Возвращает сгенерированные байты или пустой вектор при ошибке.
std::vector<uint8_t> bf_ui_generate_and_save(const std::string& filepath,
                                              size_t byteCount,
                                              const std::string& label);

// Загрузить байты из файла (ключ или IV).
// При ошибке выводит сообщение и возвращает пустой вектор.
std::vector<uint8_t> bf_ui_load_from_file(const std::string& filepath,
                                           const std::string& label);
>>>>>>> BlowTEA
