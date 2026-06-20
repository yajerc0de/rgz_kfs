#include <iostream>
#include <string>
#include <limits>
#include <fstream>
#include <functional>

using namespace std;

// ─── Прототипы точек входа в модули ──────────────────────────────────────────
void runTEA();
// ─────────────────────────────────────────────────────────────────────────────

// =============================================================================
//  Файл для хранения хэша пароля
// =============================================================================

static const string PASSWORD_FILE = "passwd.cfg";

// Простой хэш — djb2, достаточно для учебного проекта
static size_t djb2(const string& s) {
    size_t hash = 5381;
    for (unsigned char c : s)
        hash = ((hash << 5) + hash) ^ c;
    return hash;
}

// Сохранить хэш пароля в файл
static bool save_password_hash(const string& password) {
    ofstream f(PASSWORD_FILE);
    if (!f) return false;
    f << djb2(password);
    return true;
}

// Загрузить хэш из файла
static bool load_password_hash(size_t& out_hash) {
    ifstream f(PASSWORD_FILE);
    if (!f) return false;
    f >> out_hash;
    return true;
}

// =============================================================================
//  Авторизация
// =============================================================================

static bool login() {
    size_t stored_hash = 0;

    // Если файла с паролем нет — первый запуск, устанавливаем пароль
    if (!load_password_hash(stored_hash)) {
        cout << "\n  [!] Файл пароля не найден. Первый запуск — установите пароль.\n";
        cout << "  Введите новый пароль: ";
        string pw1, pw2;
        getline(cin, pw1);
        cout << "  Повторите пароль    : ";
        getline(cin, pw2);

        if (pw1 != pw2 || pw1.empty()) {
            cout << "\n  [!] Пароли не совпадают или пустой пароль. Выход.\n";
            return false;
        }

        if (!save_password_hash(pw1)) {
            cout << "\n  [!] Не удалось сохранить пароль. Проверьте права доступа.\n";
            return false;
        }

        cout << "  [OK] Пароль установлен.\n";
        return true;
    }

    // Пароль уже установлен — проверяем
    const int MAX_ATTEMPTS = 3;
    for (int attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {
        cout << "\n  Введите пароль (попытка " << attempt << "/" << MAX_ATTEMPTS << "): ";
        string pw;
        getline(cin, pw);

        if (djb2(pw) == stored_hash) {
            cout << "  [OK] Авторизация успешна.\n";
            return true;
        }

        cout << "  [!] Неверный пароль.\n";
    }

    cout << "\n  [!] Превышено число попыток. Доступ запрещён.\n";
    return false;
}

// =============================================================================
//  UI главного меню
// =============================================================================

namespace UI {

void print_separator(char ch = '-', int width = 50) {
    cout << string(width, ch) << "\n";
}

void print_header() {
    print_separator('=');
    cout << "   Encryption Algorithm RGR\n";
    cout << "   Шифрование и дешифрование данных\n";
    print_separator('=');
    cout << "\n";
}

void print_menu() {
    cout << "  Доступные алгоритмы шифрования:\n\n";
    cout << "    1.  Speck\n";
    cout << "    2.  RC5\n";
    cout << "    3.  AES-128\n";
    cout << "    4.  Serpent\n";
    cout << "    5.  Blowfish\n";
    cout << "    6.  TEA  (Tiny Encryption Algorithm)\n";
    cout << "\n";
    cout << "    0.  Выход\n";
    cout << "\n";
    print_separator();
}

int read_choice() {
    int choice = -1;
    cout << "  Введите номер алгоритма: ";
    cin >> choice;

    if (cin.fail()) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return -1;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return choice;
}

void print_unknown_choice() {
    cout << "\n  [!] Неверный выбор. Введите число от 0 до 6.\n\n";
}

} // namespace UI


// =============================================================================
//  main
// =============================================================================

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    UI::print_header();

    if (!login()) {
        cout << "\n  Завершение работы.\n";
        return 1;
    }

    cout << "\n";

    bool running = true;
    while (running) {
        UI::print_menu();

        int choice = UI::read_choice();

        switch (choice) {
            case 1: runTEA();      break;
            case 0:
                running = false;
                cout << "\n  До свидания!\n";
                break;
            default:
                UI::print_unknown_choice();
                break;
        }
    }

    return 0;
}
