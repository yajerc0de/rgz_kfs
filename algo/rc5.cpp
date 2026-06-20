#include "rc5.h"
 
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <fstream>      // для записи в файл лога
#include <sstream>      // для удобного построения текстовых строк
#include <iomanip>      // для красивого hex-вывода (00, 1A и т.п.)
 
using namespace std;
 
 
// ======================================================================
//                         ПРОСТОЙ ЛОГГЕР (для "малышей" :))
// ----------------------------------------------------------------------
// Идея простая: на каждый важный шаг алгоритма мы дописываем одну
// понятную строчку в текстовый файл rc5_log.txt, который создаётся
// в той папке, откуда ЗАПУЩЕНА программа (рабочая директория).
//
// Если файл вдруг не открылся (например, нет прав на запись) -
// программа НЕ ломается, просто лог не пишется. Само шифрование
// и дешифрование от лога никак не зависят.
// ======================================================================
 
static const char* RC5_LOG_FILENAME = "rc5_log.txt";
 
// Номер очередной строки лога - просто для удобства, чтобы видеть
// порядок событий.
static long long rc5LogLineNumber = 0;
 
// Флаг: открывали ли мы уже лог в ЭТОМ запуске программы.
// При первом обращении мы ОЧИЩАЕМ старый файл (чтобы не путать
// текущий запуск со старыми логами), при всех следующих - дописываем.
static bool rc5LoggerStarted = false;
 
// Готовит файл лога к работе: если это первый вызов за время работы
// программы - стираем старое содержимое и пишем "шапку".
static void rc5LogEnsureFileReady() {
    if (rc5LoggerStarted)
        return;
 
    ofstream freshLog(RC5_LOG_FILENAME, ios::out | ios::trunc);
    if (freshLog.is_open()) {
        freshLog << "===================================================\n";
        freshLog << " НОВЫЙ ЗАПУСК ПРОГРАММЫ RC5\n";
        freshLog << " Здесь по шагам записано всё, что делает шифр:\n";
        freshLog << " как собираются ключи, блоки, раунды шифрования и т.д.\n";
        freshLog << "===================================================\n\n";
    }
    rc5LoggerStarted = true;
}
 
// Дописывает одну строку в лог. Каждый раз открываем файл в режиме
// "дописать в конец" (ios::app), пишем строку и сразу закрываем -
// так строка точно попадёт на диск, даже если программа потом упадёт.
static void rc5Log(const string& message) {
    rc5LogEnsureFileReady();
 
    ofstream logFile(RC5_LOG_FILENAME, ios::app);
    if (!logFile.is_open())
        return; // не вышло - ну и ладно, на работу шифра это не влияет
 
    rc5LogLineNumber++;
    logFile << "[" << rc5LogLineNumber << "] " << message << "\n";
}
 
// Превращает одно 32-битное число в красивую hex-строку вида "3F2A07C1"
static string rc5Uint32ToHex(uint32_t value) {
    ostringstream oss;
    oss << hex << uppercase << setw(8) << setfill('0') << value;
    return oss.str();
}
 
// Превращает массив байт в красивую hex-строку вида "AA BB CC DD"
static string rc5BytesToHex(const uint8_t* data, size_t len) {
    ostringstream oss;
    for (size_t i = 0; i < len; i++) {
        oss << hex << uppercase << setw(2) << setfill('0') << int(data[i]);
        if (i + 1 < len) oss << " ";
    }
    return oss.str();
}
 
static string rc5BytesToHex(const vector<uint8_t>& data) {
    return rc5BytesToHex(data.data(), data.size());
}
 
 
// ======================================================================
//                         НИЗКОУРОВНЕВЫЕ "КИРПИЧИКИ"
// ======================================================================
 
// Циклический сдвиг влево: биты, которые "вылезают" слева,
// возвращаются с правого края числа (как будто число - кольцо).
static uint32_t rc5Rotl(uint32_t x, uint32_t s) {
    s &= (RC5_W - 1);
    if (s == 0) return x;
    return (x << s) | (x >> (RC5_W - s));
}
 
// То же самое, но в другую сторону - циклический сдвиг вправо.
static uint32_t rc5Rotr(uint32_t x, uint32_t s) {
    s &= (RC5_W - 1);
    if (s == 0) return x;
    return (x >> s) | (x << (RC5_W - s));
}
 
// Берёт два "слова" A и B (по 4 байта каждое) и аккуратно
// раскладывает их в массив из 8 байт - это и есть один блок RC5.
static void rc5PackBlock(uint32_t A, uint32_t B, uint8_t* out) {
    out[0] = (A      ) & 0xFF; out[1] = (A >>  8) & 0xFF;
    out[2] = (A >> 16) & 0xFF; out[3] = (A >> 24) & 0xFF;
    out[4] = (B      ) & 0xFF; out[5] = (B >>  8) & 0xFF;
    out[6] = (B >> 16) & 0xFF; out[7] = (B >> 24) & 0xFF;
}
 
// Обратная операция: берёт 8 байт блока и собирает из них
// обратно два 32-битных слова A и B.
static void rc5UnpackBlock(const uint8_t* in, uint32_t& A, uint32_t& B) {
    A =  uint32_t(in[0])
      | (uint32_t(in[1]) <<  8)
      | (uint32_t(in[2]) << 16)
      | (uint32_t(in[3]) << 24);
    B =  uint32_t(in[4])
      | (uint32_t(in[5]) <<  8)
      | (uint32_t(in[6]) << 16)
      | (uint32_t(in[7]) << 24);
}
 
 
// ======================================================================
//                         PKCS#7 ПАДДИНГ
// ----------------------------------------------------------------------
// RC5 шифрует данные строго блоками по 8 байт. Если сообщение не
// делится на 8 без остатка - его нужно "дополнить" до целого числа
// блоков. PKCS#7 - правило, КАК дополнять: в конец дописывается N
// одинаковых байт со значением N (где N - сколько байт не хватало
// до полного блока). Это позволяет потом точно понять, сколько
// "лишних" байт надо отрезать после расшифровки.
// ======================================================================
 
static vector<uint8_t> rc5Pkcs7Pad(const vector<uint8_t>& data) {
    uint8_t padLen = static_cast<uint8_t>(RC5_BLOCK_LEN - (data.size() % RC5_BLOCK_LEN));
 
    rc5Log("Паддинг (PKCS#7): исходных данных " + to_string(data.size()) +
           " байт, добавляем " + to_string(int(padLen)) +
           " байт со значением 0x" + rc5BytesToHex(&padLen, 1));
 
    vector<uint8_t> padded(data);
    padded.insert(padded.end(), padLen, padLen);
 
    rc5Log("Паддинг готов: итоговый размер " + to_string(padded.size()) +
           " байт (кратен " + to_string(RC5_BLOCK_LEN) + ")");
 
    return padded;
}
 
static vector<uint8_t> rc5Pkcs7Unpad(const vector<uint8_t>& data) {
    if (data.empty() || data.size() % RC5_BLOCK_LEN != 0) {
        rc5Log("ОШИБКА: размер данных при снятии паддинга некорректен (" +
               to_string(data.size()) + " байт)");
        throw runtime_error("RC5: неверный размер данных при снятии паддинга");
    }
 
    uint8_t padLen = data.back();
    if (padLen == 0 || padLen > RC5_BLOCK_LEN) {
        rc5Log("ОШИБКА: последний байт паддинга = " + to_string(int(padLen)) +
               " - это невозможное значение для PKCS#7");
        throw runtime_error("RC5: повреждён PKCS#7 паддинг");
    }
 
    for (size_t i = data.size() - padLen; i < data.size(); i++) {
        if (data[i] != padLen) {
            rc5Log("ОШИБКА: байты паддинга не совпадают друг с другом - "
                   "скорее всего, использован неверный ключ");
            throw runtime_error("RC5: неверный PKCS#7 паддинг (возможно, неверный ключ)");
        }
    }
 
    rc5Log("Паддинг снят: убрали " + to_string(int(padLen)) +
           " байт, осталось " + to_string(data.size() - padLen) + " байт полезных данных");
 
    return vector<uint8_t>(data.begin(), data.end() - padLen);
}
 
 
// ======================================================================
//                  РАСШИРЕНИЕ КЛЮЧА (key schedule)
// ----------------------------------------------------------------------
// Маленький ключ, который вводит пользователь (например, 16 байт),
// нужно "размазать" в большую таблицу S из RC5_TABLE_SIZE 32-битных
// слов. Эта таблица потом используется при шифровании/дешифровке
// каждого блока. Делается это в три шага:
//   1) ключ режется на 32-битные слова и складывается в массив L;
//   2) таблица S заполняется "магическими" константами P и Q;
//   3) S и L несколько раз "перемешиваются" друг с другом.
// ======================================================================
 
static void rc5ExpandKey(Rc5KeySchedule& sched, const vector<uint8_t>& key) {
    const int b = static_cast<int>(key.size());
    const int u = RC5_W / 8;
    const int c = (b + u - 1) / u;
 
    rc5Log("Расширение ключа: длина ключа b=" + to_string(b) +
           " байт, размер слова u=" + to_string(u) +
           " байт, число слов в ключе c=" + to_string(c));
 
    // Шаг 1: режем байты ключа на 32-битные слова L[0..c-1]
    vector<uint32_t> L(c, 0);
    for (int i = b - 1; i >= 0; i--)
        L[i / u] = (L[i / u] << 8) + key[i];
 
    {
        ostringstream oss;
        for (int i = 0; i < c; i++) {
            oss << rc5Uint32ToHex(L[i]);
            if (i + 1 < c) oss << " ";
        }
        rc5Log("Ключ разложен на слова L: " + oss.str());
    }
 
    // Шаг 2: заполняем таблицу S магическими константами P и Q
    sched.S[0] = RC5_P32;
    for (int i = 1; i < RC5_TABLE_SIZE; i++)
        sched.S[i] = sched.S[i - 1] + RC5_Q32;
 
    rc5Log("Таблица S заполнена константами P=0x" + rc5Uint32ToHex(RC5_P32) +
           " и Q=0x" + rc5Uint32ToHex(RC5_Q32) +
           " (всего " + to_string(RC5_TABLE_SIZE) + " слов)");
 
    // Шаг 3: перемешиваем S и L друг с другом много раз подряд
    uint32_t A = 0, B = 0;
    int i = 0, j = 0;
    int iterations = 3 * max(RC5_TABLE_SIZE, c);
 
    rc5Log("Начинаем перемешивание S и L: всего " + to_string(iterations) + " шагов");
 
    for (int k = 0; k < iterations; k++) {
        sched.S[i] = rc5Rotl(sched.S[i] + A + B, 3);
        A = sched.S[i];
        i = (i + 1) % RC5_TABLE_SIZE;
 
        L[j] = rc5Rotl(L[j] + A + B, (A + B) & (RC5_W - 1));
        B = L[j];
        j = (j + 1) % c;
 
        rc5Log("  Перемешивание, шаг " + to_string(k + 1) + "/" + to_string(iterations) +
               ": A=" + rc5Uint32ToHex(A) + ", B=" + rc5Uint32ToHex(B));
    }
 
    rc5Log("Расширение ключа завершено, таблица S готова к работе");
}
 
 
 
void rc5InitSchedule(Rc5KeySchedule& sched) {
    rc5Log("Инициализация расписания ключей: таблица S обнуляется, ключ ещё не установлен");
    memset(sched.S, 0, sizeof(sched.S));
    sched.keyIsSet = false;
}
 
bool rc5SetKey(Rc5KeySchedule& sched, const vector<uint8_t>& key) {
    rc5Log("Установка ключа: получен ключ длиной " + to_string(key.size()) + " байт");
 
    if (key.empty() || key.size() > 255) {
        rc5Log("ОШИБКА: ключ отвергнут (длина должна быть от 1 до 255 байт)");
        return false;
    }
 
    rc5ExpandKey(sched, key);
    sched.keyIsSet = true;
 
    rc5Log("Ключ успешно установлен и готов к использованию");
    return true;
}
 
