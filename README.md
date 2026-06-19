# РГР: Шифрование и дешифрование файлов и текста

Консольное приложение на C++, реализующее шифрование/дешифрование текста и
файлов шестью симметричными блочными алгоритмами. Каждый алгоритм собирается
в отдельную динамическую библиотеку (`.dll` / `.so`) и подключается к главной
программе в рантайме.

При первом запуске программа предлагает установить пароль. При последующих
запусках требует его ввод (не более трёх попыток). Хэш пароля хранится
в `passwd.cfg`.

---

## Структура проекта

```
rgz_kfs/
│
├── CMakeLists.txt              ← главный сборочный файл (собирает всё сразу)
├── .gitignore
├── README.md                   ← этот файл
├── passwd.cfg                  ← хэш пароля (создаётся при первом запуске)
│
├── main.cpp                    ← точка входа: авторизация + меню выбора шифра
│
├── core/                       ← общая инфраструктура, не привязанная к шифру
│   ├── loader.h                ← кросс-платформенный загрузчик .dll/.so
│   └── loader.cpp
│
├── AES/
│   ├── README.md
│   ├── algo/
│   │   ├── aes_tables.h / .cpp ← S-Box, Inv S-Box, Rcon
│   │   ├── aes_core.h / .cpp   ← SubBytes, ShiftRows, MixColumns, KeyExpansion
│   │   └── aes_cbc.h / .cpp    ← CBC + PKCS#7 поверх encrypt_block/decrypt_block
│   ├── capi/
│   │   ├── aes_capi.h          ← extern "C" + AESHandle
│   │   └── aes_capi.cpp        ← struct AESContext + new/delete
│   └── ui/
│       ├── aes_module_api.h / .cpp  ← struct AESModule + aes_load / aes_is_ready
│       ├── aes_utils.h / .cpp       ← утилиты ключей/файлов/HEX
│       ├── aes_ui.h                 ← AppMode, InputSource, прототипы
│       └── aes_ui.cpp               ← aes_run_ui()
│
├── Serpent/
│   ├── README.md
│   ├── algo/
│   │   ├── serpent_tables.h / .cpp  ← 8 S-блоков, таблицы раундов
│   │   ├── serpent_core.h / .cpp    ← Key Schedule, раунды, битовое расслоение
│   │   └── serpent_cbc.h / .cpp     ← CBC + PKCS#7
│   ├── capi/
│   │   ├── serpent_capi.h      ← extern "C" + SerpentHandle
│   │   └── serpent_capi.cpp    ← struct SerpentContext
│   └── ui/
│       ├── serpent_module_api.h / .cpp  ← struct SerpentModule + загрузка
│       ├── serpent_utils.h / .cpp       ← утилиты файлов/HEX
│       ├── serpent_ui.h                 ← SerpentAppMode, прототипы
│       └── serpent_ui.cpp               ← serpent_run_ui()
│
├── RC5/
│   ├── README.md
│   ├── algo/
│   │   ├── rc5.h               ← struct Rc5KeySchedule + прототипы
│   │   └── rc5.cpp             ← Key Schedule, раунды, CBC
│   ├── capi/
│   │   ├── rc5_capi.h          ← extern "C" + Rc5Handle
│   │   └── rc5_capi.cpp        ← struct Rc5Context
│   └── ui/
│       ├── rc5_module_api.h / .cpp  ← struct Rc5Module + загрузка
│       ├── rc5_utils.h / .cpp       ← утилиты с префиксом rc5_
│       └── rc5_ui.cpp               ← runRC5()
│
├── Speck/
│   ├── README.md
│   ├── algo/
│   │   ├── speck.h             ← struct SpeckKeySchedule + прототипы
│   │   └── speck.cpp           ← Key Schedule, ARX-раунды, CBC
│   ├── capi/
│   │   ├── speck_capi.h        ← extern "C" + SpeckHandle
│   │   └── speck_capi.cpp      ← struct SpeckContext
│   └── ui/
│       ├── speck_module_api.h / .cpp  ← struct SpeckModule + загрузка
│       ├── speck_utils.h              ← утилиты с префиксом speck_
│       ├── spec_utils.cpp             ← реализация утилит
│       └── speck_ui.cpp               ← runSpeck()
│
├── Blowfish/
│   ├── README.md
│   ├── algo/
│   │   ├── blowfish.h          ← struct BFKey + прототипы свободных функций
│   │   └── blowfish.cpp        ← таблицы π + реализация алгоритма
│   ├── capi/
│   │   ├── blowfish_capi.h     ← extern "C" + BlowfishHandle
│   │   └── blowfish_capi.cpp   ← struct BlowfishContext + malloc/free
│   └── ui/
│       ├── blowfish_module_api.h / .cpp  ← struct BlowfishModule + blowfish_bind_symbol<>
│       ├── blowfish_utils.h / .cpp       ← утилиты с префиксом bf_ui_
│       └── blowfish_ui.cpp               ← runBlowfish()
│
├── TEA/
│   ├── README.md
│   ├── algo/
│   │   ├── tea.h               ← struct TeaKey + прототипы свободных функций
│   │   └── tea.cpp             ← реализация алгоритма TEA
│   ├── capi/
│   │   ├── tea_capi.h          ← extern "C" + TEAHandle
│   │   └── tea_capi.cpp        ← struct TEAContext + malloc/free
│   └── ui/
│       ├── tea_module_api.h / .cpp  ← struct TEAModule + tea_bind_symbol<>
│       ├── tea_utils.h / .cpp       ← утилиты с префиксом tea_
│       └── tea_ui.cpp               ← runTEA()
│
├── Testobjects/                ← тестовые файлы (txt, jpg, png, pptx, docx, pdf)
│
└── (создаются программой при первом запуске, не хранятся в репозитории)
    ├── passwd.cfg              ← хэш пароля (djb2)
    ├── Encryptfiles/           ← зашифрованные файлы
    ├── Decryptfiles/           ← расшифрованные файлы
    ├── aes_key.bin / aes_iv.bin
    ├── serpent_key.bin / serpent_iv.bin
    ├── rc5_key.bin / rc5_iv.bin
    ├── speck_key.bin / speck_iv.bin
    ├── blowfish_key.bin / blowfish_iv.bin
    └── tea_key.bin / tea_iv.bin
```

---

## Архитектура

Каждый шифр разбит на три слоя:

**`algo/`** — чистая реализация алгоритма. Никаких классов: только свободные
функции и `struct` для хранения состояния ключа. Не знает о консоли, файлах
или динамических библиотеках. Попадает только в `.dll`/`.so`.

**`capi/`** — C-совместимая обёртка (`extern "C"`) над функциями из `algo/`.
Обеспечивает стабильный ABI: C++ манглинг имён не применяется, что позволяет
загружать библиотеку любым загрузчиком. Внутри — `struct Context` и тонкие
обёртки. Тоже попадает только в `.dll`/`.so`.

**`ui/`** — консольный интерфейс: меню, ввод текста, работа с файлами,
генератор ключей. Загружает `.dll`/`.so` в рантайме через `core/loader`
и вызывает функции через указатели в `struct Module`. Попадает только в `app`.

**`core/loader`** — единственный файл, общий для всех шифров:
кросс-платформенная обёртка над `LoadLibrary`/`GetProcAddress` (Windows)
и `dlopen`/`dlsym` (Linux/Mac).

---

## Ключевые решения

**Нет классов.** Вместо классов используются структуры данных без логики
(только поля) и свободные функции. Примеры: `struct TeaKey`, `struct BFKey`,
`struct Rc5KeySchedule`, `struct SpeckKeySchedule` — хранят состояние ключа.
`struct TEAContext`, `struct BlowfishContext` и т.д. — внутренние контексты
capi-слоя, аллоцируемые через `malloc`/`free` (или `new`/`delete` для AES).

**Нет namespace.** Конфликты имён при линковке в одном `.exe` устраняются
префиксами функций:

| Модуль   | Префикс утилит | Префикс algo |
|----------|----------------|--------------|
| AES      | (без префикса) | (без префикса) |
| Serpent  | (без префикса) | `serpent`    |
| RC5      | `rc5_`         | `rc5`        |
| Speck    | `speck_`       | `speck`      |
| Blowfish | `bf_ui_`       | `bf_`        |
| TEA      | `tea_`         | `tea_`       |

**Нет исключений на границе DLL.** В `capi/` при ошибке возвращается `0`
или пустой вектор вместо исключений. Исключения используются только внутри
`algo/` (AES, RC5, Speck) и не пересекают границу библиотеки.

**Авторизация по паролю.** Функция `login()` в `main.cpp` при первом
запуске сохраняет хэш пароля (алгоритм djb2) в `passwd.cfg`. При
последующих запусках предоставляется три попытки ввода.

---

## Форматы зашифрованных файлов

Разные модули используют разные форматы хранения метаданных:

| Модуль         | Формат .bin файла                                    |
|----------------|------------------------------------------------------|
| TEA, Blowfish, RC5, Speck | `[4 байта длина имени][N байт имя][IV][шифротекст]` |
| AES, Serpent   | `[16 байт IV][1 байт длина расширения][расширение][шифротекст]` |

---

## Сборка

Рекомендуемый способ — через CMake, собирает все 6 библиотек и главную
программу одной командой:

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"   # Windows
# или cmake ..                  # Linux
cmake --build .
```

После сборки в папке `build/` будут лежать `app.exe` (или `app`) и все
`.dll`/`.so` рядом — готовы к запуску.

---

## Алгоритмы

| Алгоритм | Блок    | Ключ        | Раундов | Структура | Режим |
|----------|---------|-------------|---------|-----------|-------|
| AES-128  | 128 бит | 128 бит     | 10      | SPN       | CBC   |
| Serpent  | 128 бит | 128 бит     | 32      | SPN       | CBC   |
| RC5      | 64 бит  | 128 бит     | 12      | Фейстель  | CBC   |
| Speck    | 128 бит | 128–256 бит | 32–34   | ARX       | CBC   |
| Blowfish | 64 бит  | 32–448 бит  | 16      | Фейстель  | CBC   |
| TEA      | 64 бит  | 128 бит     | 32×2    | Фейстель  | CBC   |

---

## Документация по модулям

- [AES/README.md](AES/README.md)
- [Serpent/README.md](Serpent/README.md)
- [RC5/README.md](RC5/README.md)
- [Speck/README.md](Speck/README.md)
- [Blowfish/README.md](Blowfish/README.md)
- [TEA/README.md](TEA/README.md)