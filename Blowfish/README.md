# Модуль Blowfish

Реализация блочного симметричного шифра Blowfish в виде динамической
библиотеки (`blowfish.dll` / `blowfish.so`) с консольным интерфейсом,
который загружает её в рантайме.

---

## Структура директорий

```
Blowfish/
├── algo/                       ← чистый алгоритм, попадает ТОЛЬКО в blowfish.dll/.so
│   ├── blowfish.h              ← struct BFKey + прототипы свободных функций
│   └── blowfish.cpp            ← таблицы π + реализация алгоритма
│
├── capi/                       ← C-обёртка для динамической библиотеки
│   ├── blowfish_capi.h         ← extern "C" + BlowfishHandle
│   └── blowfish_capi.cpp       ← struct BlowfishContext + malloc/free
│
└── ui/                         ← консольный интерфейс, попадает ТОЛЬКО в app
    ├── blowfish_module_api.h   ← struct BlowfishModule + blowfish_bind_symbol<>
    ├── blowfish_module_api.cpp ← blowfish_load / blowfish_is_ready / blowfish_last_error
    ├── blowfish_utils.h        ← утилиты с префиксом bf_ui_
    ├── blowfish_utils.cpp
    └── blowfish_ui.cpp         ← runBlowfish()
```

Дополнительно используется общий файл вне папки `Blowfish/`:

```
core/
├── loader.h                    ← кросс-платформенный загрузчик .dll/.so
└── loader.cpp                    (общий для всех шифров, не дублируется)
```

---

## За что отвечает каждый файл

### `algo/blowfish.h`

Объявляет константы алгоритма на уровне файла (`BF_ROUNDS`, `BF_P_ARRAY_SIZE`,
`BF_S_BOX_COUNT`, `BF_S_BOX_SIZE`, `BF_BLOCK_BYTES`, `BF_KEY_MIN`,
`BF_KEY_MAX`) и структуру `BFKey` — хранит P-массив из 18 слов, 4 S-блока
по 256 слов и флаг `ready`. Никакой логики внутри структуры.

Объявляет внешние таблицы инициализации `BF_INIT_P` / `BF_INIT_S` и
свободные функции:
- `bf_key_init` / `bf_key_set` — инициализация и Key Schedule
- `bf_encrypt_block` / `bf_decrypt_block` — блочные операции
- `bf_cbc_encrypt` / `bf_cbc_decrypt` — CBC-режим

### `algo/blowfish.cpp`

Реализация алгоритма Blowfish без классов:

- **Константы `BF_INIT_P` / `BF_INIT_S`** — дробные части числа π (~4 КБ
  таблиц). Определены один раз как глобальные константы.
- **`bf_key_init`** — копирует `BF_INIT_P` / `BF_INIT_S` в `BFKey`, сбрасывает
  `ready = false`.
- **`bf_key_set` (Key Schedule)** — сброс P/S к константам π, XOR P-массива
  с байтами ключа (циклически), затем 521 шифрование нулевого блока для
  генерации финальных P и S. Принимает `BFKey*` вместо `this`.
- **`bf_f`** (static) — функция F: нелинейное преобразование через S-блоки:
  `((S[0][a] + S[1][b]) ^ S[2][c]) + S[3][d]`. Принимает `const BFKey*`.
- **`bf_encrypt_block` / `bf_decrypt_block`** — 16 раундов сети Фейстеля.
  Принимают `BFKey*` и указатели на `L`, `R`.
- **`bf_pkcs7_pad` / `bf_pkcs7_unpad`** (static) — дополнение до кратности
  8 байт и снятие паддинга. При ошибке возвращают пустой вектор.
- **`bf_pack_block` / `bf_unpack_block`** (static) — упаковка двух `uint32_t`
  в 8 байт и обратно (big-endian).
- **`bf_cbc_encrypt` / `bf_cbc_decrypt`** — CBC-режим для данных произвольной
  длины. При ошибке возвращают пустой вектор.

Этот файл **не знает** о консоли, файлах или динамических библиотеках.

### `capi/blowfish_capi.h`

C-совместимый интерфейс модуля. Объявляет `extern "C"` функции и
непрозрачный указатель `BlowfishHandle`. Макрос `BLOWFISH_API` превращается
в `__declspec(dllexport)` при сборке библиотеки и в `__declspec(dllimport)`
при использовании из `app`.

Функции:
- `blowfish_create` / `blowfish_destroy` — создание и уничтожение контекста
- `blowfish_set_key` — установка ключа + Key Schedule
- `blowfish_encrypt_cbc` / `blowfish_decrypt_cbc` — шифрование/дешифрование
- `blowfish_free_buffer` — освобождение буфера результата

### `capi/blowfish_capi.cpp`

Реализация C-интерфейса. Внутри — `struct BlowfishContext` с полем `BFKey key`.
Контекст создаётся через `malloc`, уничтожается через `free` — никакого
C++ рантайма на границе библиотеки.

- **`blowfish_create`** — `malloc(sizeof(BlowfishContext))` + `bf_key_init()`
- **`blowfish_destroy`** — `free(handle)`
- **`blowfish_set_key`** — вызывает `bf_key_set()`, запускает Key Schedule
- **`blowfish_encrypt_cbc` / `blowfish_decrypt_cbc`** — вызывают
  `bf_cbc_encrypt` / `bf_cbc_decrypt`. Результат копируется в `malloc`-буфер.
  При пустом результате возвращают `0`.
- **`blowfish_free_buffer`** — `free(buffer)`

### `ui/blowfish_module_api.h` / `blowfish_module_api.cpp`

`struct BlowfishModule` — хранит `DynamicLibrary library` и шесть указателей
на функции (`create`, `destroy`, `setKey`, `encryptCbc`, `decryptCbc`,
`freeBuffer`). Никакой логики внутри структуры.

Свободные функции:
- **`blowfish_load(module, path)`** — загружает `blowfish.dll`/`blowfish.so`
  и привязывает все символы через `blowfish_bind_symbol<>`. При неудаче
  выгружает библиотеку.
- **`blowfish_is_ready(module)`** — проверяет что все указатели заполнены.
- **`blowfish_last_error(module)`** — текст последней ошибки.
- **`blowfish_bind_symbol<FnPtr>(module, name, target)`** — шаблонная функция
  привязки одного символа, реализована в заголовке.

### `ui/blowfish_utils.h` / `blowfish_utils.cpp`

Свободные функции с префиксом `bf_ui_` — префикс выбран чтобы не конфликтовать
с algo-функциями (`bf_*`) при линковке в одном `.exe`:

- **`bf_ui_hex_to_bytes` / `bf_ui_bytes_to_hex`** — конвертация HEX ↔ байты
- **`bf_ui_random_bytes`** — генерация случайных байт через `mt19937`
- **`bf_ui_read_file` / `bf_ui_write_file`** — бинарное чтение/запись файлов
- **`bf_ui_ensure_dir`** — создание папки (`stat`/`mkdir`, без `<filesystem>`)
- **`bf_ui_extract_filename`** — извлечение имени файла из пути
- **`bf_ui_extract_extension`** — извлечение расширения с точкой (`.jpg`)
- **`bf_ui_build_encrypt_path` / `bf_ui_build_decrypt_path`** — пути в папках
  `Encryptfiles/` и `Decryptfiles/` с авто-созданием папок
- **`bf_ui_pack_filename_header` / `bf_ui_unpack_filename_header`** — упаковка
  оригинального имени файла в бинарный заголовок формата
  `[4 байта длина][N байт имя]`
- **`bf_ui_generate_and_save` / `bf_ui_load_from_file`** — генерация/загрузка
  ключа или IV с выводом HEX в консоль

### `ui/blowfish_ui.cpp`

Точка входа `runBlowfish()`, вызываемая из `main.cpp`. Не знает о `BFKey`
напрямую — вся работа через `BlowfishModule`:

1. Загружает `blowfish.dll`/`blowfish.so` через `blowfish_load(&mod, LIB_PATH)`.
2. Показывает меню: текст / файл / генератор ключей.
3. **Текстовый режим** — шифрует введённый текст, выводит шифротекст
   в HEX; при расшифровке запрашивает HEX шифротекста, IV берёт из файла.
4. **Файловый режим** — шифрует файл в `.bin` формат с встроенными
   метаданными `[заголовок имени][8 байт IV][шифротекст]`, сохраняет в
   `Encryptfiles/`. При расшифровке восстанавливает оригинальное имя
   и сохраняет в `Decryptfiles/`.
5. **Генератор ключей** — запрашивает длину ключа от 4 до 56 байт
   (Blowfish поддерживает переменную длину в отличие от TEA) и сохраняет
   в `blowfish_key.bin`.

---

## Сборка

### Windows (MinGW / g++)

```bash
g++ -std=c++17 -shared -o blowfish.dll Blowfish/algo/blowfish.cpp Blowfish/capi/blowfish_capi.cpp -IBlowfish/algo -IBlowfish/capi -DBLOWFISH_BUILD_DLL
g++ -std=c++17 -o app.exe main.cpp core/loader.cpp Blowfish/ui/blowfish_ui.cpp Blowfish/ui/blowfish_utils.cpp Blowfish/ui/blowfish_module_api.cpp -Icore -IBlowfish/ui -IBlowfish/capi
```

### Linux (g++)

```bash
g++ -std=c++17 -shared -fPIC -o blowfish.so Blowfish/algo/blowfish.cpp Blowfish/capi/blowfish_capi.cpp -IBlowfish/algo -IBlowfish/capi
g++ -std=c++17 -o app main.cpp core/loader.cpp Blowfish/ui/blowfish_ui.cpp Blowfish/ui/blowfish_utils.cpp Blowfish/ui/blowfish_module_api.cpp -Icore -IBlowfish/ui -IBlowfish/capi -ldl
```

**`blowfish.dll` / `blowfish.so` должна лежать рядом с `app.exe` / `app`**
при запуске.

### Через CMake (рекомендуется)

```bash
mkdir build && cd build && cmake .. && cmake --build .
```

---

## Параметры алгоритма

| Параметр      | Значение                           |
|---------------|------------------------------------|
| Размер блока  | 64 бита (8 байт)                   |
| Длина ключа   | 32–448 бит (4–56 байт, переменная) |
| Число раундов | 16                                 |
| Режим работы  | CBC (Cipher Block Chaining)        |
| Паддинг       | PKCS#7                             |