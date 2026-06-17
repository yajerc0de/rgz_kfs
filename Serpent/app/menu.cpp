#include "menu.h"
#include "serpent_core.h"
#include "serpent_cbc.h"
#include "utils.h"

#include <iostream>
#include <string>
#include <vector>
#include <limits>

// Директория, куда сохраняются результаты - текущая папка программы
static std::string get_output_dir() {
    return "./";
}

// Выбор источника данных для шифрования
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
    std::cout << "     Serpent CBC Шифратор/Дешифратор    \n";
    std::cout << "========================================\n";
    std::cout << "  [1] Зашифровать\n";
    std::cout << "  [2] Расшифровать\n";
    std::cout << "  [0] Выход\n";
    std::cout << "  Ваш выбор: ";

    std::string choice = "";
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (choice == "1") return AppMode::ENCRYPT;
    if (choice == "2") return AppMode::DECRYPT;
    if (choice == "0") return AppMode::EXIT;
    return AppMode::UNKNOWN;
}

// Режим шифрования

void run_encrypt_mode() {
    std::cout << "\n--- РЕЖИМ ШИФРОВАНИЯ ---\n";

    InputSource src = ask_input_source();

    std::vector<unsigned char> plaintext;
    std::string out_filename;     // Имя выходного зашифрованного файла
    std::string original_ext;     // Расширение исходного файла (для последующего восстановления)

    if (src == InputSource::TEXT_CONSOLE) {
        std::cout << "Введите текст для шифрования:\n> ";
        std::string text;
        std::getline(std::cin, text);

        if (text.empty()) {
            std::cout << "[Ошибка] Текст не может быть пустым\n";
            return;
        }

        plaintext.assign(text.begin(), text.end());
        out_filename = "encrypted_text.bin";
        original_ext = "txt"; // текст с консоли при расшифровке восстановится как .txt

    } else {
        std::cout << "Введите путь к файлу: ";
        std::string filepath;
        std::getline(std::cin, filepath);

        if (!read_binary_file(filepath, plaintext)) return;

        // Запоминаем расширение исходного файла, чтобы восстановить его при дешифровке
        original_ext = get_file_extension(filepath);

        // Имя выходного файла: имя_исходного_encrypted.bin
        out_filename = get_file_name_without_extension(filepath) + "_encrypted.bin";
    }

    // Генерируем ключ и сохраняем в key.bin рядом с программой
    unsigned char cipher_key[KEY_SIZE];
    std::string key_path = get_output_dir() + "key.bin";
    if (!generate_and_save_key(key_path, cipher_key)) return;

    // Разворачиваем подключи Serpent
    unsigned int subkeys[SUBKEYS_COUNT];
    serpent_expand_key(cipher_key, subkeys);

    // Генерируем IV
    unsigned char iv[BLOCK_SIZE];
    generate_iv(iv);
    print_hex("Вектор инициализации IV (HEX)", iv, BLOCK_SIZE);

    // Шифруем данные
    std::cout << "\nШифрование...\n";
    std::vector<unsigned char> ciphertext = serpent_cbc_encrypt(plaintext, iv, subkeys);

    // Сохраняем: IV + расширение + зашифрованные данные в один файл
    std::string out_path = get_output_dir() + out_filename;
    if (!save_encrypted_file(out_path, iv, original_ext, ciphertext)) return;

    std::cout << "\n[OK] Зашифрованный файл сохранён: " << out_path << "\n";
    std::cout << "[OK] Ключ сохранён: " << key_path << "\n";
    std::cout << "     Исходное расширение: " << (original_ext.empty() ? "(нет)" : original_ext) << "\n";
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
    unsigned char cipher_key[KEY_SIZE];
    if (!load_key(key_path, cipher_key)) return;

    print_hex("Загружен ключ (HEX)", cipher_key, KEY_SIZE);

    // Разворачиваем подключи Serpent
    unsigned int subkeys[SUBKEYS_COUNT];
    serpent_expand_key(cipher_key, subkeys);

    // Загружаем IV, расширение и зашифрованные данные из файла
    unsigned char iv[BLOCK_SIZE];
    std::string original_ext;
    std::vector<unsigned char> ciphertext;
    if (!load_encrypted_file(enc_path, iv, original_ext, ciphertext)) return;

    print_hex("IV из файла (HEX)", iv, BLOCK_SIZE);
    std::cout << "Исходное расширение: " << (original_ext.empty() ? "(нет)" : original_ext) << "\n";
    std::cout << "Размер зашифрованных данных: " << ciphertext.size() << " байт\n";

    // Расшифровываем
    std::cout << "\nРасшифровка...\n";
    std::vector<unsigned char> plaintext = serpent_cbc_decrypt(ciphertext, iv, subkeys);

    if (plaintext.empty()) {
        std::cout << "[Ошибка] Результат расшифровки пустой - возможно, неверный ключ\n";
        return;
    }

    // Формируем имя выходного файла, восстанавливая исходное расширение
    std::string base_name = get_file_name_without_extension(enc_path);

    // Убираем суффикс "_encrypted" из имени, если он есть, чтобы имя было чище
    std::string suffix = "_encrypted";
    int base_len = (int)base_name.size();
    int suf_len = (int)suffix.size();
    if (base_len > suf_len && base_name.substr(base_len - suf_len) == suffix) {
        base_name = base_name.substr(0, base_len - suf_len);
    }

    std::string out_filename = base_name + "_decrypted";
    if (!original_ext.empty())
        out_filename += "." + original_ext;

    std::string out_path = get_output_dir() + out_filename;

    if (!write_binary_file(out_path, plaintext)) return;

    std::cout << "\n[OK] Расшифрованный файл сохранён: " << out_path << "\n";
    std::cout << "     Размер расшифрованных данных: " << plaintext.size() << " байт\n";

    // Если расширение - txt (или его нет) и данные похожи на текст, показываем превью
    bool looks_like_text = (original_ext == "txt" || original_ext.empty());
    if (looks_like_text) {
        int preview_len = (int)plaintext.size();
        if (preview_len > 200) preview_len = 200;

        for (int i = 0; i < preview_len; ++i) {
            unsigned char c = plaintext[i];
            if (c < 0x09 || (c > 0x0D && c < 0x20)) {
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
}
