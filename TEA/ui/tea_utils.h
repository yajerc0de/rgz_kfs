#pragma once

#include <string>
#include <vector>
#include <cstdint>

using namespace std;

// =============================================================================
//  tea_utils.h — утилиты для модуля TEA
//  Конвертация HEX, работа с файлами, генерация случайных байт
// =============================================================================

// ─── HEX ─────────────────────────────────────────────────────────────────────

// Конвертировать hex-строку ("a1b2c3...") в вектор байт.
// Возвращает false если строка содержит недопустимые символы или нечётная длина.
bool hexToBytes(const string& hex, vector<uint8_t>& out);

// Конвертировать вектор байт в hex-строку нижнего регистра.
string bytesToHex(const vector<uint8_t>& data);

// ─── Случайные байты ─────────────────────────────────────────────────────────

// Сгенерировать count случайных байт через mt19937 + random_device.
vector<uint8_t> randomBytes(size_t count);

// ─── Файлы ───────────────────────────────────────────────────────────────────

// Считать содержимое файла в бинарном режиме.
// При ошибке выводит сообщение в cout и возвращает false.
bool readFile(const string& path, vector<uint8_t>& data);

// Записать байты в файл в бинарном режиме.
// При ошибке выводит сообщение в cout и возвращает false.
bool writeFile(const string& path, const vector<uint8_t>& data);

// Создать папку если она не существует.
// Возвращает false при ошибке создания.
bool ensureDir(const string& dirPath);

// Извлечь имя файла без пути (например "docs/test.txt" -> "test.txt").
string extractFilename(const string& path);

// Собрать путь для зашифрованного файла.
// Результат: Encryptfiles/encrypted_<имя файла без расширения>.bin
// Расширение всегда .bin — оригинальное имя/расширение хранится внутри файла.
// Папка создаётся автоматически если не существует.
string buildEncryptPath(const string& sourcePath);

// Собрать путь для расшифрованного файла.
// originalName — оригинальное имя, извлечённое из метаданных внутри .bin файла.
// Результат: Decryptfiles/decrypted_<originalName>
// Папка создаётся автоматически если не существует.
string buildDecryptPath(const string& originalName);

// ─── Метаданные имени файла (для шифрования файлов) ──────────────────────────

// Упаковать оригинальное имя файла в бинарный префикс:
// [4 байта: длина имени (little-endian uint32_t)][N байт: имя файла UTF-8]
vector<uint8_t> packFilenameHeader(const string& originalFilename);

// Распаковать имя файла из начала буфера.
// Возвращает true при успехе; originalFilename получает извлечённое имя,
// bytesConsumed — сколько байт занял заголовок (4 + длина имени).
// Возвращает false если данных недостаточно или длина некорректна.
bool unpackFilenameHeader(const vector<uint8_t>& data,
                           string& originalFilename,
                           size_t& bytesConsumed);

// ─── Ключи и IV ──────────────────────────────────────────────────────────────

// Сгенерировать случайные байты, сохранить в файл и вывести HEX в консоль.
// Возвращает сгенерированные байты или пустой вектор при ошибке.
vector<uint8_t> generateAndSave(const string& filepath, size_t byteCount, const string& label);

// Загрузить байты из файла (ключ или IV).
// При ошибке выводит сообщение и возвращает пустой вектор.
vector<uint8_t> loadFromFile(const string& filepath, const string& label);