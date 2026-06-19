#pragma once

enum class SerpentInputSource {
    TEXT_CONSOLE,
    BINARY_FILE
};

enum class SerpentAppMode {
    ENCRYPT,
    DECRYPT,
    UNKNOWN,
    EXIT
};

SerpentAppMode serpent_show_main_menu();

void serpent_run_encrypt_mode();

void serpent_run_decrypt_mode();

void serpent_run_ui();
