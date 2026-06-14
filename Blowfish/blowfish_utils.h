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

// Encryptfiles/encrypted_<имя файла>  (папка создаётся автоматически)
string buildEncryptPath(const string& sourcePath);

// Decryptfiles/decrypted_<имя файла>  (папка создаётся автоматически)
string buildDecryptPath(const string& sourcePath);

// ─── Ключи и IV ──────────────────────────────────────────────────────────────

vector<uint8_t> generateAndSave(const string& filepath, size_t byteCount, const string& label);
vector<uint8_t> loadFromFile(const string& filepath, const string& label);