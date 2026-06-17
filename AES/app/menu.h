#pragma once

// Варианты источника данных для шифрования
enum class InputSource {
    TEXT_CONSOLE,
    BINARY_FILE
};

// Режимы работы программы
enum class AppMode {
    ENCRYPT,
    DECRYPT,
    UNKNOWN,
    EXIT
};

// Показывает главное меню и возвращает выбранный режим
AppMode show_main_menu();

// Режим шифрования
void run_encrypt_mode();

// Режим дешифрования
void run_decrypt_mode();
