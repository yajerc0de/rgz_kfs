#include <iostream>
#include <string>
#include <limits>


using namespace std;

// ─── Прототипы точек входа в модули (будут реализованы позже) ────────────────
void runSpeck();
void runRC5();
void runAES();
void runSerpent();
void runBlowfish();
void runTEA();
// ─────────────────────────────────────────────────────────────────────────────

namespace UI {

// Горизонтальный разделитель
void printSeparator(char ch = '-', int width = 50) {
    cout << string(width, ch) << "\n";
}

void printHeader() {
    printSeparator('=');
    cout << "   Encryption Algorithm RGR\n";
    cout << "   Шифрование и дешифрование данных\n";
    printSeparator('=');
    cout << "\n";
}

void printMenu() {
    cout << "  Доступные алгоритмы шифрования:\n\n";
    cout << "    1.  Speck\n";
    cout << "    2.  RC5\n";
    cout << "    3.  AES\n";
    cout << "    4.  Serpent\n";
    cout << "    5.  Blowfish\n";
    cout << "    6.  TEA  (Tiny Encryption Algorithm)\n";
    cout << "\n";
    cout << "    0.  Выход\n";
    cout << "\n";
    printSeparator();
}

// Возвращает выбранный пункт меню (0–6), при ошибке ввода возвращает -1
int readChoice() {
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

void printUnknownChoice() {
    cout << "\n  [!] Неверный выбор. Введите число от 0 до 6.\n\n";
}

void printNotImplemented(const string& name) {
    cout << "\n";
    printSeparator();
    cout << "  Модуль «" << name << "» ещё не подключён.\n";
    printSeparator();
    cout << "\n";
}

} // namespace UI

// ─── Заглушки модулей ─────────────────────────────────────────────────────────
// Когда модуль будет реализован — удалите его заглушку отсюда
// и подключите соответствующий заголовочный файл вверху.

void runSpeck()    { UI::printNotImplemented("Speck");    }
void runRC5()      { UI::printNotImplemented("RC5");      }
void runAES()      { UI::printNotImplemented("AES");      }
void runSerpent()  { UI::printNotImplemented("Serpent");  }
void runTEA()      { UI::printNotImplemented("TEA");      }
// ─────────────────────────────────────────────────────────────────────────────

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    UI::printHeader();

    bool running = true;
    while (running) {
        UI::printMenu();

        int choice = UI::readChoice();

        switch (choice) {
            case 1: runSpeck();    break;
            case 2: runRC5();      break;
            case 3: runAES();      break;
            case 4: runSerpent();  break;
            case 5: runBlowfish(); break;
            case 6: runTEA();      break;
            case 0:
                running = false;
                break;
            default:
                UI::printUnknownChoice();
                break;
        }
    }

    return 0;
}