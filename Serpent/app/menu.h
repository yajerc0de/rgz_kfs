#pragma once

// Источник данных для шифрования
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

// Режим шифрования: спрашивает источник данных, шифрует и сохраняет файл + ключ
void run_encrypt_mode();

// Режим дешифрования: спрашивает файл и ключ, расшифровывает и сохраняет результат
void run_decrypt_mode();
