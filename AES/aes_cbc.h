#pragma once

#include <vector>

// Шифрование данных в режиме CBC.
// iv      — вектор инициализации (16 байт)
// round_keys — раундовые ключи (из expand_key)
// Возвращает зашифрованные данные (с PKCS#7 паддингом)
std::vector<unsigned char> cbc_encrypt(
    const std::vector<unsigned char>& plaintext,
    const unsigned char* iv,
    const unsigned char* round_keys
);

// Дешифрование данных в режиме CBC.
// Возвращает исходные данные (паддинг снимается автоматически)
std::vector<unsigned char> cbc_decrypt(
    const std::vector<unsigned char>& ciphertext,
    const unsigned char* iv,
    const unsigned char* round_keys
);
