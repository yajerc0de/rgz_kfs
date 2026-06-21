#include "rc5_module_api.h"
#include "rc5_utils.h"

#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <iomanip>
#include <sstream>
#include <cstdint>

using namespace std;

static const string KEY_FILE = "rc5_key.bin";
static const string IV_FILE  = "rc5_iv.bin";

#ifdef _WIN32
static const string LIB_PATH = "rc5.dll";
#else
static const string LIB_PATH = "./rc5.so";
#endif

static void printSep(char ch = '-', int w = 50) {
    cout << string(w, ch) << "\n";
}


//  Вспомогательные функции для логирования RC5


static string hex32(uint32_t v) {
    ostringstream oss;
    oss << "0x" << uppercase << hex << setw(8) << setfill('0') << v;
    return oss.str();
}

// Циклический сдвиг влево 32-бит (для лога)
static uint32_t rc5_rotl(uint32_t x, uint32_t n) {
    n &= 31;
    if (n == 0) return x;
    return (x << n) | (x >> (32 - n));
}

static uint32_t rc5_rotr(uint32_t x, uint32_t n) {
    n &= 31;
    if (n == 0) return x;
    return (x >> n) | (x << (32 - n));
}

// Key Schedule для лога 
static void rc5_log_key_schedule(
    const vector<uint8_t>& keyBytes,
    uint32_t S[26],
    ofstream& log)
{
    const uint32_t P32 = 0xB7E15163u;
    const uint32_t Q32 = 0x9E3779B9u;
    const int ROUNDS = 12;
    const int t = 2 * (ROUNDS + 1); // 26

    // Этап 1: загрузка ключа в L
    uint32_t L[4] = {0, 0, 0, 0};
    int b = (int)keyBytes.size();
    for (int i = b - 1; i >= 0; --i)
        L[i / 4] = (L[i / 4] << 8) + keyBytes[i];

    log << "=== KEY SCHEDULE ===\n\n";
    log << "Ключ (" << b << " байт): ";
    for (uint8_t byte : keyBytes)
        log << uppercase << hex << setw(2) << setfill('0') << (int)byte << " ";
    log << dec << "\n\n";

    log << "Этап 1 — загрузка в L (little-endian):\n";
    for (int i = 0; i < 4; ++i)
        log << "  L[" << i << "] = " << hex32(L[i]) << "\n";
    log << "\n";

    // Этап 2: инициализация S
    S[0] = P32;
    for (int i = 1; i < t; ++i)
        S[i] = S[i - 1] + Q32;

    log << "Этап 2 — инициализация S (P32=0xB7E15163, Q32=0x9E3779B9):\n";
    for (int i = 0; i < t; ++i)
        log << "  S[" << setw(2) << i << "] = " << hex32(S[i]) << "\n";
    log << dec << "\n";

    // Этап 3: смешивание
    log << "Этап 3 — смешивание (78 итераций):\n";
    uint32_t A = 0, B = 0;
    int ii = 0, jj = 0;
    for (int iter = 0; iter < 3 * t; ++iter) {
        A = S[ii] = rc5_rotl((S[ii] + A + B), 3);
        B = L[jj] = rc5_rotl((L[jj] + A + B), (A + B) & 31);
        log << "  iter " << setw(2) << dec << iter
            << "  S[" << setw(2) << ii << "]=" << hex32(A)
            << "  L[" << jj << "]=" << hex32(B) << "\n";
        ii = (ii + 1) % t;
        jj = (jj + 1) % 4;
    }
    log << "\nИтоговые подключи S[0..25]:\n";
    for (int i = 0; i < t; ++i)
        log << "  S[" << setw(2) << dec << i << "] = " << hex32(S[i]) << "\n";
    log << "\n";
}

// Один раунд шифрования RC5 с логом
static void rc5_log_encrypt_round(
    uint32_t& A, uint32_t& B,
    const uint32_t S[26],
    int round,
    ofstream& log)
{
    uint32_t old_A = A, old_B = B;

    uint32_t xor_ab  = A ^ B;
    uint32_t shift_b = B & 31;
    uint32_t rot     = rc5_rotl(xor_ab, shift_b);
    A = rot + S[2 * round];

    uint32_t xor_ba  = B ^ A;
    uint32_t shift_a = A & 31;
    uint32_t rot2    = rc5_rotl(xor_ba, shift_a);
    B = rot2 + S[2 * round + 1];

    log << "Раунд " << setw(2) << round << ":\n";
    log << "  A до        = " << hex32(old_A) << "\n";
    log << "  B до        = " << hex32(old_B) << "\n";
    log << "  A XOR B     = " << hex32(xor_ab) << "\n";
    log << "  сдвиг (B&31)= " << dec << shift_b << "\n";
    log << "  rotl(A^B,B) = " << hex32(rot) << "\n";
    log << "  S[" << setw(2) << 2*round << "]      = " << hex32(S[2*round]) << "\n";
    log << "  A после     = " << hex32(A) << "\n";
    log << "  ---\n";
    log << "  B XOR A_new = " << hex32(xor_ba) << "\n";
    log << "  сдвиг (A&31)= " << dec << shift_a << "\n";
    log << "  rotl(B^A,A) = " << hex32(rot2) << "\n";
    log << "  S[" << setw(2) << 2*round+1 << "]      = " << hex32(S[2*round+1]) << "\n";
    log << "  B после     = " << hex32(B) << "\n\n";
}

// Один раунд дешифрования RC5 с логом
static void rc5_log_decrypt_round(
    uint32_t& A, uint32_t& B,
    const uint32_t S[26],
    int round,
    ofstream& log)
{
    uint32_t old_A = A, old_B = B;

    uint32_t sub_b  = B - S[2 * round + 1];
    uint32_t shift_a = A & 31;
    B = rc5_rotr(sub_b, shift_a) ^ A;

    uint32_t sub_a  = A - S[2 * round];
    uint32_t shift_b = B & 31;
    A = rc5_rotr(sub_a, shift_b) ^ B;

    log << "Раунд " << setw(2) << round << " (обратный):\n";
    log << "  A до        = " << hex32(old_A) << "\n";
    log << "  B до        = " << hex32(old_B) << "\n";
    log << "  B-S[" << setw(2) << 2*round+1 << "]   = " << hex32(sub_b) << "\n";
    log << "  сдвиг(A&31) = " << dec << shift_a << "\n";
    log << "  B после     = " << hex32(B) << "\n";
    log << "  ---\n";
    log << "  A-S[" << setw(2) << 2*round << "]   = " << hex32(sub_a) << "\n";
    log << "  сдвиг(B&31) = " << dec << shift_b << "\n";
    log << "  A после     = " << hex32(A) << "\n\n";
}


//  Режим с логом: шифрование файла


static void modeFileLogEncrypt(
    Rc5Module& mod, Rc5Handle handle,
    const vector<uint8_t>& keyBytes)
{
    cout << "\n  Путь к исходному файлу: ";
    string inPath;
    getline(cin, inPath);

    vector<uint8_t> plain;
    if (!rc5_readFile(inPath, plain)) return;

    string outPath = rc5_buildEncryptPath(inPath);
    string logPath = "rc5_encrypt_log.txt";

    vector<uint8_t> iv = rc5_generateAndSave(IV_FILE, RC5_BLOCK_BYTES, "IV");
    if (iv.empty()) return;


    uint8_t* outData = nullptr;
    size_t   outLen  = 0;

    int ok = mod.encryptCbc(handle, plain.data(), plain.size(),
                            iv.data(), &outData, &outLen);
    if (!ok) { cout << "\n  [!] Ошибка шифрования файла.\n"; return; }

    vector<uint8_t> cipher(outData, outData + outLen);
    mod.freeBuffer(outData);

    string originalName = rc5_extractFilename(inPath);
    vector<uint8_t> nameHeader = rc5_packFilenameHeader(originalName);

    vector<uint8_t> output;
    output.insert(output.end(), nameHeader.begin(), nameHeader.end());
    output.insert(output.end(), iv.begin(),         iv.end());
    output.insert(output.end(), cipher.begin(),     cipher.end());

    if (!rc5_writeFile(outPath, output)) return;

    // Лог 
    ofstream log(logPath);
    log << "=== RC5-32/12/16 ШИФРОВАНИЕ ФАЙЛА ===\n\n";
    log << "Исходный файл : " << inPath << "\n";
    log << "Размер данных : " << plain.size() << " байт\n";
    log << "IV (HEX)      : " << rc5_bytesToHex(iv) << "\n\n";

    uint32_t S[26];
    rc5_log_key_schedule(keyBytes, S, log);

    vector<uint8_t> padded = plain;
    uint8_t padLen = (uint8_t)(8 - (plain.size() % 8));
    padded.insert(padded.end(), padLen, padLen);

    log << "=== CBC ШИФРОВАНИЕ БЛОКОВ ===\n\n";
    log << "Паддинг PKCS#7: +" << (int)padLen
        << " байт (0x" << hex << (int)padLen << dec << ")\n";
    log << "Всего блоков: " << padded.size() / 8 << "\n\n";

    uint8_t prev[8];
    for (int b = 0; b < 8; ++b) prev[b] = iv[b];

    size_t total = padded.size() / 8;

    for (size_t blk = 0; blk < total; ++blk) {
        log << "=========================================\n";
        log << "БЛОК " << (blk + 1) << " из " << total << "\n";
        log << "=========================================\n";

        log << "Байты открытого текста: ";
        for (int b = 0; b < 8; ++b)
            log << uppercase << hex << setw(2) << setfill('0')
                << (int)padded[blk*8+b] << " ";
        log << "\n";

        // XOR с предыдущим блоком
        uint8_t xored[8];
        log << "XOR с предыдущим блоком (CBC):\n";
        for (int b = 0; b < 8; ++b) {
            xored[b] = padded[blk*8+b] ^ prev[b];
            log << "  байт[" << dec << b << "]: "
                << uppercase << hex << setw(2) << setfill('0') << (int)padded[blk*8+b]
                << " XOR "
                << setw(2) << setfill('0') << (int)prev[b]
                << " = "
                << setw(2) << setfill('0') << (int)xored[b] << "\n";
        }

        // Распаковка little-endian в A и B
        uint32_t A = (uint32_t)xored[0] | ((uint32_t)xored[1]<<8)
                   | ((uint32_t)xored[2]<<16) | ((uint32_t)xored[3]<<24);
        uint32_t B = (uint32_t)xored[4] | ((uint32_t)xored[5]<<8)
                   | ((uint32_t)xored[6]<<16) | ((uint32_t)xored[7]<<24);

        log << dec << "\nA начальное = " << hex32(A) << "\n";
        log << "B начальное = " << hex32(B) << "\n\n";

        log << "Pre-whitening:\n";
        log << "  A = A + S[0] = " << hex32(A) << " + " << hex32(S[0]);
        A += S[0];
        log << " = " << hex32(A) << "\n";
        log << "  B = B + S[1] = " << hex32(B) << " + " << hex32(S[1]);
        B += S[1];
        log << " = " << hex32(B) << "\n\n";

        // 12 раундов
        for (int r = 1; r <= 12; ++r)
            rc5_log_encrypt_round(A, B, S, r, log);

        log << "A итог = " << hex32(A) << "\n";
        log << "B итог = " << hex32(B) << "\n";

        uint8_t enc[8];
        enc[0]=A&0xFF; enc[1]=(A>>8)&0xFF; enc[2]=(A>>16)&0xFF; enc[3]=(A>>24)&0xFF;
        enc[4]=B&0xFF; enc[5]=(B>>8)&0xFF; enc[6]=(B>>16)&0xFF; enc[7]=(B>>24)&0xFF;

        log << "Шифроблок: ";
        for (int b = 0; b < 8; ++b)
            log << uppercase << hex << setw(2) << setfill('0') << (int)enc[b] << " ";
        log << "\n\n";

        for (int b = 0; b < 8; ++b) prev[b] = enc[b];
    }

    log << "=== ШИФРОВАНИЕ ЗАВЕРШЕНО ===\n";
    log << "Зашифрованный файл: " << outPath << "\n";
    log.close();

    cout << "\n";
    printSep();
    cout << "  Исходный файл  : " << inPath       << "\n";
    cout << "  Зашифрован в   : " << outPath      << "\n";
    cout << "  Лог шагов      : " << logPath      << "\n";
    cout << "  Размер         : " << plain.size() << " -> " << output.size() << " байт\n";
    printSep();
}

//  Режим с логом: дешифрование файла

static void modeFileLogDecrypt(
    Rc5Module& mod, Rc5Handle handle,
    const vector<uint8_t>& keyBytes)
{
    cout << "\n  Путь к зашифрованному файлу: ";
    string inPath;
    getline(cin, inPath);

    vector<uint8_t> raw;
    if (!rc5_readFile(inPath, raw)) return;

    string originalName;
    size_t headerSize = 0;
    if (!rc5_unpackFilenameHeader(raw, originalName, headerSize)) {
        cout << "\n  [!] Не удалось прочитать имя файла из метаданных.\n";
        return;
    }

    if (raw.size() < headerSize + (size_t)RC5_BLOCK_BYTES) {
        cout << "\n  [!] Файл слишком мал или повреждён.\n";
        return;
    }

    vector<uint8_t> iv(raw.begin() + headerSize,
                        raw.begin() + headerSize + RC5_BLOCK_BYTES);
    vector<uint8_t> cipher(raw.begin() + headerSize + RC5_BLOCK_BYTES,
                            raw.end());

    string outPath = rc5_buildDecryptPath(originalName);
    string logPath = "rc5_decrypt_log.txt";

   
    uint8_t* outData = nullptr;
    size_t   outLen  = 0;

    int ok = mod.decryptCbc(handle, cipher.data(), cipher.size(),
                            iv.data(), &outData, &outLen);
    if (!ok) { cout << "\n  [!] Ошибка расшифровки файла.\n"; return; }

    vector<uint8_t> plain(outData, outData + outLen);
    mod.freeBuffer(outData);

    if (!rc5_writeFile(outPath, plain)) return;

  
    ofstream log(logPath);
    log << "=== RC5-32/12/16 ДЕШИФРОВАНИЕ ФАЙЛА ===\n\n";
    log << "Файл          : " << inPath << "\n";
    log << "Размер шифра  : " << cipher.size() << " байт\n";
    log << "IV (HEX)      : " << rc5_bytesToHex(iv) << "\n\n";

    uint32_t S[26];
    rc5_log_key_schedule(keyBytes, S, log);

    log << "=== CBC ДЕШИФРОВАНИЕ БЛОКОВ ===\n\n";
    log << "Всего блоков: " << cipher.size() / 8 << "\n\n";

    uint8_t prev[8];
    for (int b = 0; b < 8; ++b) prev[b] = iv[b];

    size_t total = cipher.size() / 8;

    for (size_t blk = 0; blk < total; ++blk) {
        log << "=========================================\n";
        log << "БЛОК " << (blk + 1) << " из " << total << "\n";
        log << "=========================================\n";

        log << "Байты шифроблока: ";
        for (int b = 0; b < 8; ++b)
            log << uppercase << hex << setw(2) << setfill('0')
                << (int)cipher[blk*8+b] << " ";
        log << "\n";

        
        uint32_t A = (uint32_t)cipher[blk*8  ] | ((uint32_t)cipher[blk*8+1]<<8)
                   | ((uint32_t)cipher[blk*8+2]<<16) | ((uint32_t)cipher[blk*8+3]<<24);
        uint32_t B = (uint32_t)cipher[blk*8+4] | ((uint32_t)cipher[blk*8+5]<<8)
                   | ((uint32_t)cipher[blk*8+6]<<16) | ((uint32_t)cipher[blk*8+7]<<24);

        log << dec << "\nA начальное = " << hex32(A) << "\n";
        log << "B начальное = " << hex32(B) << "\n\n";

        // 12 обратных раундов
        for (int r = 12; r >= 1; --r)
            rc5_log_decrypt_round(A, B, S, r, log);

        log << "Снятие pre-whitening:\n";
        log << "  B = B - S[1] = " << hex32(B) << " - " << hex32(S[1]);
        B -= S[1];
        log << " = " << hex32(B) << "\n";
        log << "  A = A - S[0] = " << hex32(A) << " - " << hex32(S[0]);
        A -= S[0];
        log << " = " << hex32(A) << "\n\n";

        uint8_t dec_b[8];
        dec_b[0]=A&0xFF; dec_b[1]=(A>>8)&0xFF;
        dec_b[2]=(A>>16)&0xFF; dec_b[3]=(A>>24)&0xFF;
        dec_b[4]=B&0xFF; dec_b[5]=(B>>8)&0xFF;
        dec_b[6]=(B>>16)&0xFF; dec_b[7]=(B>>24)&0xFF;

        // XOR с предыдущим шифроблоком (CBC)
        log << "XOR с предыдущим блоком (CBC):\n";
        for (int b = 0; b < 8; ++b) {
            uint8_t res = dec_b[b] ^ prev[b];
            log << "  байт[" << dec << b << "]: "
                << uppercase << hex << setw(2) << setfill('0') << (int)dec_b[b]
                << " XOR "
                << setw(2) << setfill('0') << (int)prev[b]
                << " = "
                << setw(2) << setfill('0') << (int)res << "\n";
        }
        log << "\n";

        for (int b = 0; b < 8; ++b) prev[b] = cipher[blk*8+b];
    }

    if (!plain.empty()) {
        uint8_t padLen = plain.back();
        log << dec << "=== СНЯТИЕ PKCS#7 ПАДДИНГА ===\n";
        log << "Последний байт = 0x" << hex << (int)padLen
            << " -> удаляем " << dec << (int)padLen << " байт\n";
        log << "Размер после снятия: " << plain.size() << " байт\n";
    }

    log << "\n=== ДЕШИФРОВАНИЕ ЗАВЕРШЕНО ===\n";
    log << "Восстановленный файл: " << outPath << "\n";
    log.close();

    cout << "\n";
    printSep();
    cout << "  Зашифрованный файл: " << inPath       << "\n";
    cout << "  Расшифрован в     : " << outPath      << "\n";
    cout << "  Лог шагов         : " << logPath      << "\n";
    cout << "  Размер данных     : " << plain.size() << " байт\n";
    printSep();
}

//  Режим 1: текст


static void modeText(Rc5Module& mod, Rc5Handle handle) {
    cout << "\n  Текстовый режим:\n";
    cout << "    1. Зашифровать текст\n";
    cout << "    2. Расшифровать текст\n";
    cout << "  Выбор: ";

    int choice = 0;
    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 1) {
        cout << "\n  Введите текст для шифрования:\n  > ";
        string text;
        getline(cin, text);

        vector<uint8_t> iv = rc5_generateAndSave(IV_FILE, RC5_BLOCK_BYTES, "IV");
        if (iv.empty()) return;

        vector<uint8_t> plain(text.begin(), text.end());
        uint8_t* outData = nullptr; size_t outLen = 0;

        int ok = mod.encryptCbc(handle, plain.data(), plain.size(),
                                iv.data(), &outData, &outLen);
        if (!ok) { cout << "\n  [!] Ошибка шифрования.\n"; return; }

        vector<uint8_t> cipher(outData, outData + outLen);
        mod.freeBuffer(outData);

        cout << "\n"; printSep();
        cout << "  Шифротекст (HEX): " << rc5_bytesToHex(cipher) << "\n";
        cout << "  IV сохранён в   : " << IV_FILE  << "\n";
        cout << "  Ключ сохранён в : " << KEY_FILE << "\n";
        printSep();

    } else if (choice == 2) {
        cout << "\n  Введите шифротекст (HEX): ";
        string hexCipher;
        cin >> hexCipher;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        vector<uint8_t> iv = rc5_loadFromFile(IV_FILE, "IV");
        if (iv.empty() || iv.size() != RC5_BLOCK_BYTES) {
            cout << "\n  [!] Неверный IV.\n"; return;
        }

        vector<uint8_t> cipher;
        if (!rc5_hexToBytes(hexCipher, cipher)) {
            cout << "\n  [!] Неверный HEX.\n"; return;
        }

        uint8_t* outData = nullptr; size_t outLen = 0;
        int ok = mod.decryptCbc(handle, cipher.data(), cipher.size(),
                                iv.data(), &outData, &outLen);
        if (!ok) { cout << "\n  [!] Ошибка расшифровки.\n"; return; }

        vector<uint8_t> plain(outData, outData + outLen);
        mod.freeBuffer(outData);

        cout << "\n"; printSep();
        cout << "  Результат: " << string(plain.begin(), plain.end()) << "\n";
        printSep();
    } else {
        cout << "\n  [!] Неверный выбор.\n";
    }
}

//  Режим 2: файл (без лога)

static void modeFile(Rc5Module& mod, Rc5Handle handle) {
    cout << "\n  Файловый режим:\n";
    cout << "    1. Зашифровать файл\n";
    cout << "    2. Расшифровать файл\n";
    cout << "  Выбор: ";

    int choice = 0;
    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 1) {
        cout << "\n  Путь к исходному файлу: ";
        string inPath; getline(cin, inPath);

        vector<uint8_t> plain;
        if (!rc5_readFile(inPath, plain)) return;

        string outPath = rc5_buildEncryptPath(inPath);
        vector<uint8_t> iv = rc5_generateAndSave(IV_FILE, RC5_BLOCK_BYTES, "IV");
        if (iv.empty()) return;

        uint8_t* outData = nullptr; size_t outLen = 0;
        int ok = mod.encryptCbc(handle, plain.data(), plain.size(),
                                iv.data(), &outData, &outLen);
        if (!ok) { cout << "\n  [!] Ошибка шифрования.\n"; return; }

        vector<uint8_t> cipher(outData, outData + outLen);
        mod.freeBuffer(outData);

        string originalName = rc5_extractFilename(inPath);
        vector<uint8_t> nameHeader = rc5_packFilenameHeader(originalName);

        vector<uint8_t> output;
        output.insert(output.end(), nameHeader.begin(), nameHeader.end());
        output.insert(output.end(), iv.begin(), iv.end());
        output.insert(output.end(), cipher.begin(), cipher.end());

        if (!rc5_writeFile(outPath, output)) return;

        cout << "\n"; printSep();
        cout << "  Зашифрован в   : " << outPath      << "\n";
        cout << "  Размер         : " << plain.size() << " -> " << output.size() << " байт\n";
        printSep();

    } else if (choice == 2) {
        cout << "\n  Путь к зашифрованному файлу: ";
        string inPath; getline(cin, inPath);

        vector<uint8_t> raw;
        if (!rc5_readFile(inPath, raw)) return;

        string originalName; size_t headerSize = 0;
        if (!rc5_unpackFilenameHeader(raw, originalName, headerSize)) {
            cout << "\n  [!] Повреждённые метаданные.\n"; return;
        }

        vector<uint8_t> iv(raw.begin() + headerSize,
                            raw.begin() + headerSize + RC5_BLOCK_BYTES);
        vector<uint8_t> cipher(raw.begin() + headerSize + RC5_BLOCK_BYTES,
                                raw.end());

        string outPath = rc5_buildDecryptPath(originalName);
        uint8_t* outData = nullptr; size_t outLen = 0;

        int ok = mod.decryptCbc(handle, cipher.data(), cipher.size(),
                                iv.data(), &outData, &outLen);
        if (!ok) { cout << "\n  [!] Ошибка расшифровки.\n"; return; }

        vector<uint8_t> plain(outData, outData + outLen);
        mod.freeBuffer(outData);
        if (!rc5_writeFile(outPath, plain)) return;

        cout << "\n"; printSep();
        cout << "  Расшифрован в  : " << outPath      << "\n";
        cout << "  Размер данных  : " << plain.size() << " байт\n";
        printSep();
    } else {
        cout << "\n  [!] Неверный выбор.\n";
    }
}

//  Режим 3: генератор ключей

static void modeKeyGen() {
    cout << "\n  Генератор ключей RC5\n";
    cout << "  Доступные варианты:\n";
    cout << "    1. 16 байт (128 бит) — рекомендуется\n";
    cout << "    2. 24 байта (192 бит)\n";
    cout << "    3. 32 байта (256 бит)\n";
    cout << "  Выбор: ";

    int choice = 0;
    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    size_t keyLen = 16;
    if (choice == 2) keyLen = 24;
    else if (choice == 3) keyLen = 32;
    else if (choice != 1)
        cout << "\n  [!] Неверный выбор. Используется 16 байт.\n";

    vector<uint8_t> key = rc5_generateAndSave(KEY_FILE, keyLen, "КЛЮЧ");
    if (key.empty()) return;

    cout << "\n"; printSep();
    cout << "  Файл : " << KEY_FILE << "\n";
    cout << "  Длина: " << keyLen << " байт (" << keyLen * 8 << " бит)\n";
    printSep();
}

//  runRC5()

void runRC5() {
    cout << "\n";
    printSep('=');
    cout << "  RC5 — Rivest Cipher 5\n";
    cout << "  Блок: 64 бит | Ключ: 128/192/256 бит | Раундов: 12 | Режим: CBC\n";
    printSep('=');

    Rc5Module mod;
    if (!rc5ModuleLoad(mod, LIB_PATH)) {
        cout << "\n  [!] Не удалось загрузить библиотеку: " << LIB_PATH << "\n";
        cout << "  Причина: " << mod.lastError << "\n";
        return;
    }
    cout << "  [OK] Библиотека " << LIB_PATH << " загружена.\n";

    bool running = true;
    while (running) {
        cout << "\n  Что вы хотите сделать?\n\n";
        cout << "    1. Зашифровать / расшифровать текст\n";
        cout << "    2. Зашифровать / расшифровать файл\n";
        cout << "    3. Сгенерировать ключ\n";
        cout << "    4. Зашифровать файл с подробным логом\n";
        cout << "    5. Расшифровать файл с подробным логом\n";
        cout << "    0. Вернуться в главное меню\n\n";
        printSep();
        cout << "  Выбор: ";

        int choice = 0;
        cin >> choice;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (choice == 0) { running = false; continue; }
        if (choice == 3) { modeKeyGen(); continue; }

        if (choice == 1 || choice == 2 || choice == 4 || choice == 5) {
            vector<uint8_t> keyBytes = rc5_loadFromFile(KEY_FILE, "КЛЮЧ");
            if (keyBytes.empty()) {
                cout << "\n  [!] Файл ключа не найден. Сначала выберите пункт 3.\n";
                continue;
            }

            Rc5Handle handle = mod.create();
            if (!handle) { cout << "\n  [!] Ошибка создания контекста RC5.\n"; continue; }

            if (!mod.setKey(handle, keyBytes.data(), keyBytes.size())) {
                cout << "\n  [!] Неверная длина ключа.\n";
                mod.destroy(handle); continue;
            }
            cout << "  [OK] Ключ загружен.\n";

            if      (choice == 1) modeText(mod, handle);
            else if (choice == 2) modeFile(mod, handle);
            else if (choice == 4) modeFileLogEncrypt(mod, handle, keyBytes);
            else if (choice == 5) modeFileLogDecrypt(mod, handle, keyBytes);

            mod.destroy(handle);
        } else {
            cout << "\n  [!] Неверный выбор.\n";
        }
    }
}
