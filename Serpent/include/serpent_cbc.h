#pragma once

#include <serpent_export.h>
#include <vector>

// Шифрование данных в режиме CBC с использованием Serpent.
// iv - вектор инициализации (16 байт)
// subkeys - развёрнутые подключи (из serpent_expand_key)
// Возвращает зашифрованные данные (с PKCS#7 паддингом)
SERPENT_API std::vector<unsigned char> serpent_cbc_encrypt(
    const std::vector<unsigned char>& plaintext,
    const unsigned char* iv,
    const unsigned int* subkeys
);

// Дешифрование данных в режиме CBC с использованием Serpent.
SERPENT_API std::vector<unsigned char> serpent_cbc_decrypt(
    const std::vector<unsigned char>& ciphertext,
    const unsigned char* iv,
    const unsigned int* subkeys
);
