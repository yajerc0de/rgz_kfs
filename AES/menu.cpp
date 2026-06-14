#include "menu.h"
#include "aes_core.h"
#include "aes_cbc.h"
#include "utils.h"

#include <iostream>
#include <string>
#include <vector>
#include <limits>

// Вспомогательные функции

static std::string get_output_dir() {
    return "./";
}

// Убирает расширение из имени файла
static std::string strip_extension(const std::string& filename) {
    // Ищем последний слэш, чтобы взять только имя файла
    int slash_pos = -1;
    for (int i = (int)filename.size() - 1; i >= 0; --i) {
        if (filename[i] == '/' || filename[i] == '\\') {
            slash_pos = i;
            break;
        }
    }
    std::string name = filename.substr(slash_pos + 1);

    // Ищем точку расширения
    int dot_pos = -1;
    for (int i = (int)name.size() - 1; i >= 0; --i) {
        if (name[i] == '.') {
            dot_pos = i;
            break;
        }
    }
    if (dot_pos > 0)
        return name.substr(0, dot_pos);
    return name;
}

static std::string get_extension(const std::string& filename) {
    int slash_pos = -1;
    for (int i = (int)filename.size() - 1; i >= 0; --i) {
        if (filename[i] == '/' || filename[i] == '\\') {
            slash_pos = i;
            break;
        }
    }

    std::string name = filename.substr(slash_pos + 1);

    int dot_pos = -1;
    for (int i = (int)name.size() - 1; i >= 0; --i) {
        if (name[i] == '.') {
            dot_pos = i;
            break;
        }
    }

    if (dot_pos >= 0)
        return name.substr(dot_pos + 1);

    return "";
}

// Выбор источника данных
static InputSource ask_input_source() {
    std::cout << "\n  Выберите источник данных:\n";
    std::cout << "  [1] Текст с клавиатуры\n";
    std::cout << "  [2] Файл (.txt, .jpg, .mp3 и любые другие)\n";
    std::cout << "  Ваш выбор: ";

    int choice = 0;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (choice == 1) return InputSource::TEXT_CONSOLE;
    return InputSource::BINARY_FILE;
}

// Главное меню

AppMode show_main_menu() {
    std::cout << "\n========================================\n";
    std::cout << "       AES-128 CBC Шифратор/Дешифратор  \n";
    std::cout << "========================================\n";
    std::cout << "  [1] Зашифровать\n";
    std::cout << "  [2] Расшифровать\n";
    std::cout << "  [0] Выход\n";
    std::cout << "  Ваш выбор: ";

    int choice = 0;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (choice == 1) return AppMode::ENCRYPT;
    if (choice == 2) return AppMode::DECRYPT;
    if (choice == 0) return AppMode::EXIT;
    return AppMode::UNKNOWN;
}

// Режим шифрования

void run_encrypt_mode() {
    std::cout << "\n--- РЕЖИМ ШИФРОВАНИЯ ---\n";

    InputSource src = ask_input_source();

    std::vector<unsigned char> plaintext;
    std::string out_filename;
    std::string extension;

    if (src == InputSource::TEXT_CONSOLE) {
        std::cout << "Введите текст для шифрования:\n> ";
        std::string text;
        std::getline(std::cin, text);

        if (text.empty()) {
            std::cout << "[Ошибка] Текст не может быть пустым\n";
            return;
        }

        plaintext.assign(text.begin(), text.end());
        extension = "txt";
        out_filename = "encrypted_text.bin";

    } else {
        std::cout << "Введите путь к файлу: ";
        std::string filepath;
        std::getline(std::cin, filepath);

        if (!read_binary_file(filepath, plaintext)) return;

        // Имя выходного файла: имя_исходного_encrypted.bin
        extension = get_extension(filepath);
        out_filename = strip_extension(filepath) + "_encrypted.bin";
    }

    // Генерируем ключ и сохраняем в key.bin рядом с программой
    unsigned char cipher_key[BLOCK_SIZE];
    std::string key_path = get_output_dir() + "key.bin";
    if (!generate_and_save_key(key_path, cipher_key)) return;

    // Разворачиваем раундовые ключи
    unsigned char round_keys[ROUND_KEYS_SIZE];
    expand_key(cipher_key, round_keys);

    // Генерируем IV
    unsigned char iv[BLOCK_SIZE];
    generate_iv(iv);
    print_hex("Вектор инициализации IV (HEX)", iv, BLOCK_SIZE);

    // Шифруем
    std::cout << "\nШифрование...\n";
    std::vector<unsigned char> ciphertext = cbc_encrypt(plaintext, iv, round_keys);

    // Сохраняем: IV + зашифрованные данные в один файл
    std::string out_path = get_output_dir() + out_filename;
    if (!save_encrypted_file(out_path, iv, extension, ciphertext)) return;

    std::cout << "\n[OK] Зашифрованный файл сохранён: " << out_path << "\n";
    std::cout << "[OK] Ключ сохранён: " << key_path << "\n";
    std::cout << "     Размер исходных данных: " << plaintext.size() << " байт\n";
    std::cout << "     Размер зашифрованных данных: " << ciphertext.size() << " байт\n";
}

// Режим дешифрования

void run_decrypt_mode() {
    std::cout << "\n--- РЕЖИМ ДЕШИФРОВАНИЯ ---\n";

    // Запрашиваем файл для расшифровки
    std::cout << "Введите путь к зашифрованному файлу (.bin): ";
    std::string enc_path;
    std::getline(std::cin, enc_path);

    // Запрашиваем файл ключа
    std::cout << "Введите путь к файлу ключа (key.bin): ";
    std::string key_path;
    std::getline(std::cin, key_path);

    // Загружаем ключ
    unsigned char cipher_key[BLOCK_SIZE];
    if (!load_key(key_path, cipher_key)) return;

    print_hex("Загружен ключ (HEX)", cipher_key, BLOCK_SIZE);

    // Разворачиваем раундовые ключи
    unsigned char round_keys[ROUND_KEYS_SIZE];
    expand_key(cipher_key, round_keys);

    // Загружаем IV и зашифрованные данные
    unsigned char iv[BLOCK_SIZE];
    std::vector<unsigned char> ciphertext;
    std::string extension;
    if (!load_encrypted_file(enc_path, iv, extension, ciphertext)) return;

    print_hex("IV из файла (HEX)", iv, BLOCK_SIZE);
    std::cout << "Размер зашифрованных данных: " << ciphertext.size() << " байт\n";

    // Расшифровываем
    std::cout << "\nРасшифровка...\n";
    std::vector<unsigned char> plaintext = cbc_decrypt(ciphertext, iv, round_keys);

    if (plaintext.empty()) {
        std::cout << "[Ошибка] Результат расшифровки пустой — возможно, неверный ключ\n";
        return;
    }

    // Имя выходного файла
    std::string out_filename =
    strip_extension(enc_path) + "_decrypted";
    if (!extension.empty()) out_filename += "." + extension;
    std::string out_path = get_output_dir() + out_filename;

    if (!write_binary_file(out_path, plaintext)) return;

    std::cout << "\n[OK] Расшифрованный файл сохранён: " << out_path << "\n";
    std::cout << "     Размер расшифрованных данных: " << plaintext.size() << " байт\n";

    // Если данные похожи на текст — показываем превью
    bool looks_like_text = true;
    int preview_len = (int)plaintext.size();
    if (preview_len > 200) preview_len = 200;

    for (int i = 0; i < preview_len; ++i) {
        unsigned char c = plaintext[i];
        // Проверяем что символ — обычный печатный ASCII или UTF-8 байт (>= 0x80)
        if (c < 0x09 || (c > 0x0D && c < 0x20 && c != 0x1B)) {
            looks_like_text = false;
            break;
        }
    }

    if (looks_like_text) {
        std::cout << "\nПревью текста (первые " << preview_len << " символов):\n";
        std::cout << "---\n";
        for (int i = 0; i < preview_len; ++i)
            std::cout << (char)plaintext[i];
        std::cout << "\n---\n";
    }
}
