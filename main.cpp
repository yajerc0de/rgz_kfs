#include <iostream>
#include <string>
#include <limits>
#include <fstream>
using namespace std;

void runRC5();

static const string PASSWORD_FILE = "passwd.cfg";

static size_t djb2(const string& s) {
    size_t hash = 5381;
    for (unsigned char c : s)
        hash = ((hash << 5) + hash) ^ c;
    return hash;
}

static bool save_password_hash(const string& password) {
    ofstream f(PASSWORD_FILE);
    if (!f) return false;
    f << djb2(password);
    return true;
}

static bool load_password_hash(size_t& out_hash) {
    ifstream f(PASSWORD_FILE);
    if (!f) return false;
    f >> out_hash;
    return true;
}

static bool login() {
    size_t stored_hash = 0;

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
            cout << "\n  [!] Не удалось сохранить пароль.\n";
            return false;
        }
        cout << "  [OK] Пароль установлен.\n";
        return true;
    }

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

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    cout << "==================================================\n";
    cout << "   Encryption Algorithm RGR\n";
    cout << "   RC5 — Rivest Cipher 5\n";
    cout << "==================================================\n\n";

    if (!login()) {
        cout << "\n  Завершение работы.\n";
        return 1;
    }

    cout << "\n";

    bool running = true;
    while (running) {
        cout << "  Доступные операции:\n\n";
        cout << "    1.  RC5 (Rivest Cipher 5)\n\n";
        cout << "    0.  Выход\n\n";
        cout << "--------------------------------------------------\n";
        cout << "  Введите номер: ";

        int choice = -1;
        cin >> choice;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            choice = -1;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice) {
            case 1: runRC5(); break;
            case 0:
                running = false;
                cout << "\n  До свидания!\n";
                break;
            default:
                cout << "\n  [!] Неверный выбор.\n\n";
                break;
        }
    }
    return 0;
}
