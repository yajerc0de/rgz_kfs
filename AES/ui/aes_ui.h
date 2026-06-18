#pragma once

enum class InputSource {
    TEXT_CONSOLE,
    BINARY_FILE
};

enum class AppMode {
    ENCRYPT,
    DECRYPT,
    UNKNOWN,
    EXIT
};

AppMode aes_show_main_menu();

void aes_run_encrypt_mode();

void aes_run_decrypt_mode();

void aes_run_ui();
