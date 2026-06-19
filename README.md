# РГР: Шифрование и дешифрование файлов и текста

Консольное приложение на C++, реализующее шифрование/дешифрование текста и
файлов несколькими симметричными блочными алгоритмами. Каждый алгоритм
собирается в отдельную динамическую библиотеку (`.dll` / `.so`) и
подключается к главной программе в рантайме.

---

## Структура проекта

```
rgz_kfs/
│
├── CMakeLists.txt              ← главный сборочный файл (собирает всё сразу)
├── .gitignore
├── README.md                   ← этот файл
│
├── main.cpp                    ← точка входа, меню выбора шифра
│
├── core/                       ← общая инфраструктура, не привязанная к шифру
│   ├── loader.h                ← кросс-платформенный загрузчик .dll/.so
│   └── loader.cpp
│
├── Blowfish/
│   ├── README.md
│   ├── algo/
│   │   ├── blowfish.h          ← struct BFKey + прототипы свободных функций
│   │   └── blowfish.cpp        ← таблицы π + реализация алгоритма
│   ├── capi/
│   │   ├── blowfish_capi.h     ← extern "C" интерфейс + BlowfishHandle
│   │   └── blowfish_capi.cpp   ← struct BlowfishContext + malloc/free
│   └── ui/
│       ├── blowfish_module_api.h    ← struct BlowfishModule + blowfish_bind_symbol<>
│       ├── blowfish_module_api.cpp  ← blowfish_load / blowfish_is_ready / blowfish_last_error
│       ├── blowfish_utils.h         ← утилиты с префиксом bf_ui_
│       ├── blowfish_utils.cpp
│       └── blowfish_ui.cpp          ← runBlowfish()
│
├── TEA/
│   ├── README.md
│   ├── algo/
│   │   ├── tea.h               ← struct TeaKey + прототипы свободных функций
│   │   └── tea.cpp             ← реализация алгоритма TEA
│   ├── capi/
│   │   ├── tea_capi.h          ← extern "C" интерфейс + TEAHandle
│   │   └── tea_capi.cpp        ← struct TEAContext + malloc/free
│   └── ui/
│       ├── tea_module_api.h    ← struct TEAModule + tea_bind_symbol<>
│       ├── tea_module_api.cpp  ← tea_load / tea_is_ready / tea_last_error
│       ├── tea_utils.h         ← утилиты с префиксом tea_
│       ├── tea_utils.cpp
│       └── tea_ui.cpp          ← runTEA()
│
├── Testobjects/                ← тестовые файлы для проверки шифрования
│
└── (создаются программой при первом запуске, не хранятся в репозитории)
    ├── Encryptfiles/
    ├── Decryptfiles/
    ├── blowfish_key.bin
    ├── blowfish_iv.bin
    ├── tea_key.bin
    └── tea_iv.bin
```

---

## Архитектура

Каждый шифр разбит на три слоя:

- **`algo/`** — чистая реализация алгоритма. Никаких классов: только
  свободные функции и `struct` для хранения состояния ключа. Не знает о
  консоли, файлах или динамических библиотеках. Попадает только в `.dll`/`.so`.

- **`capi/`** — C-совместимая обёртка (`extern "C"`) над функциями из `algo/`.
  Необходима потому что C++ имена функций подвергаются манглингу, а ABI
  разных компиляторов несовместим. Внутри — `struct Context` (аллоцируется
  через `malloc`, не `new`) и тонкие обёртки. Тоже попадает только в `.dll`/`.so`.

- **`ui/`** — консольный интерфейс: меню, ввод текста, работа с файлами,
  генератор ключей. Загружает соответствующую `.dll`/`.so` в рантайме через
  `core/loader` и вызывает функции из `capi/` через указатели функций в
  `struct Module`. Попадает только в `app`.

`core/loader` — единственный файл, общий для всех шифров: кросс-платформенная
обёртка над `LoadLibrary`/`GetProcAddress` (Windows) и `dlopen`/`dlsym`
(Linux/Mac).

---

## Ключевые решения

**Нет классов.** Вместо классов используются:
- `struct TeaKey` / `struct BFKey` — хранят состояние ключа (только данные)
- `struct TEAContext` / `struct BlowfishContext` — внутренние контексты в capi,
  аллоцируются через `malloc`/`free`
- `struct TEAModule` / `struct BlowfishModule` — хранят указатели на функции
  из `.dll`/`.so`

**Нет namespace.** Вместо `namespace TeaUtils` и `namespace BlowfishUtils`
используются префиксы функций: `tea_*` для TEA-утилит, `bf_ui_*` для
Blowfish-утилит (префикс `bf_` уже занят algo-функциями).

**Нет исключений на границе DLL.** В `capi/` исключения не бросаются —
при ошибке возвращается `0` или пустой вектор.

---

## Сборка

Рекомендуемый способ — через CMake, собирает все библиотеки и главную
программу одной командой:

```bash
mkdir build && cd build && cmake .. && cmake --build .
```

После сборки в папке `build/` будут лежать `app`/`app.exe` и все `.dll`/`.so`
рядом друг с другом — готовы к запуску.

Подробные инструкции по сборке отдельных модулей через `g++` — в README
каждого шифра.

---

## Документация по модулям

- [Blowfish/README.md](Blowfish/README.md)
- [TEA/README.md](TEA/README.md)
