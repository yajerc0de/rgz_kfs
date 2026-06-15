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
// Результат: Encryptfiles/encrypted_<имя файла>
// Папка создаётся автоматически если не существует.
string buildEncryptPath(const string& sourcePath);

// Собрать путь для расшифрованного файла.
// Результат: Decryptfiles/decrypted_<имя файла без префикса encrypted_>
// Папка создаётся автоматически если не существует.
string buildDecryptPath(const string& sourcePath);

// ─── Ключи и IV ──────────────────────────────────────────────────────────────

// Сгенерировать случайные байты, сохранить в файл и вывести HEX в консоль.
// Возвращает сгенерированные байты или пустой вектор при ошибке.
vector<uint8_t> generateAndSave(const string& filepath, size_t byteCount, const string& label);

// Загрузить байты из файла (ключ или IV).
// При ошибке выводит сообщение и возвращает пустой вектор.
vector<uint8_t> loadFromFile(const string& filepath, const string& label);