#include <iostream>
#include <locale>
#include "menu.h"

int main() {
    // Настройка кодировки для корректного вывода русского текста
    std::locale::global(std::locale(""));

    AppMode mode = AppMode::UNKNOWN;

    do {
        mode = show_main_menu();

        switch (mode)
        {
        case AppMode::ENCRYPT:
            run_encrypt_mode();
            break;
        case AppMode::DECRYPT:
            run_decrypt_mode();
            break;
        case AppMode::UNKNOWN:
            std::cout << "Неизвестная команда" << std::endl;
            break;
        default:
            break;
        }

    } while (mode != AppMode::EXIT);
    return 0;
}
