#pragma once

#include <string>
#include <vector>
#include <cstdint>

using namespace std;

// =============================================================================
//  blowfish_utils.h — утилиты для модуля Blowfish
//  Конвертация HEX, работа с файлами, генерация случайных байт
//  Не использует <filesystem> — совместимо с MinGW и старыми компиляторами
// =============================================================================

// ─── HEX ─────────────────────────────────────────────────────────────────────

bool hexToBytes(const string& hex, vector<uint8_t>& out);
string bytesToHex(const vector<uint8_t>& data);

// ─── Случайные байты ──────────────────────────────────────────────────────────

vector<uint8_t> randomBytes(size_t count);

// ─── Файлы ───────────────────────────────────────────────────────────────────

bool readFile(const string& path, vector<uint8_t>& data);
bool writeFile(const string& path, const vector<uint8_t>& data);

// Создать папку если не существует (WinAPI на Windows, mkdir на Linux/Mac)
bool ensureDir(const string& dirPath);

// Извлечь имя файла из пути: "docs/photo.jpg" -> "photo.jpg"
string extractFilename(const string& path);

// Извлечь расширение с точкой: "photo.jpg" -> ".jpg"
string extractExtension(const string& path);

// Собрать путь для зашифрованного файла.
// Результат: Encryptfiles/encrypted_<имя файла без расширения>.bin
// Расширение всегда .bin — оригинальное имя/расширение хранится внутри файла.
string buildEncryptPath(const string& sourcePath);

// Собрать путь для расшифрованного файла.
// originalName — оригинальное имя, извлечённое из метаданных внутри .bin файла.
// Результат: Decryptfiles/decrypted_<originalName>
string buildDecryptPath(const string& originalName);

// ─── Метаданные имени файла (для шифрования файлов) ──────────────────────────

// Упаковать оригинальное имя файла в бинарный префикс:
// [4 байта: длина имени (little-endian uint32_t)][N байт: имя файла UTF-8]
vector<uint8_t> packFilenameHeader(const string& originalFilename);

// Распаковать имя файла из начала буфера.
// Возвращает true при успехе; originalFilename получает извлечённое имя,
// bytesConsumed — сколько байт занял заголовок (4 + длина имени).
bool unpackFilenameHeader(const vector<uint8_t>& data,
                           string& originalFilename,
                           size_t& bytesConsumed);

// ─── Ключи и IV ──────────────────────────────────────────────────────────────

vector<uint8_t> generateAndSave(const string& filepath, size_t byteCount, const string& label);
vector<uint8_t> loadFromFile(const string& filepath, const string& label);