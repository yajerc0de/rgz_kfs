# Модуль TEA (Tiny Encryption Algorithm)

Реализация блочного симметричного шифра TEA в виде динамической библиотеки
(`tea.dll` / `tea.so`) с консольным интерфейсом, который загружает её в рантайме.

---

## Структура директорий

```
TEA/
├── algo/                       ← чистый алгоритм, попадает ТОЛЬКО в tea.dll/.so
│   ├── tea.h                   ← struct TeaKey + прототипы свободных функций
│   └── tea.cpp                 ← реализация алгоритма
│
├── capi/                       ← C-обёртка для динамической библиотеки
│   ├── tea_capi.h              ← extern "C" + TEAHandle
│   └── tea_capi.cpp            ← struct TEAContext + malloc/free
│
└── ui/                         ← консольный интерфейс, попадает ТОЛЬКО в app
    ├── tea_module_api.h        ← struct TEAModule + tea_bind_symbol<>
    ├── tea_module_api.cpp      ← tea_load / tea_is_ready / tea_last_error
    ├── tea_utils.h             ← утилиты с префиксом tea_
    ├── tea_utils.cpp
    └── tea_ui.cpp              ← runTEA()
```

Дополнительно используется общий файл вне папки `TEA/`:

```
core/
├── loader.h                    ← кросс-платформенный загрузчик .dll/.so
└── loader.cpp                    (общий для всех шифров, не дублируется)
```

---

## За что отвечает каждый файл

### `algo/tea.h`

Объявляет константы алгоритма на уровне файла (`TEA_DELTA`, `TEA_ROUNDS`,
`TEA_KEY_BYTES`, `TEA_BLOCK_BYTES_ALGO`) и структуру `TeaKey` — хранит
4 слова ключа `uint32_t` и флаг `ready`. Никакой логики внутри структуры.

Объявляет свободные функции:
- `tea_key_init` / `tea_key_set` — инициализация и загрузка ключа
- `tea_encrypt_block` / `tea_decrypt_block` — блочные операции
- `tea_cbc_encrypt` / `tea_cbc_decrypt` — CBC-режим

### `algo/tea.cpp`

Реализация алгоритма TEA без классов:

- **`tea_key_set`** — разбивает 16-байтный ключ на 4 слова `uint32_t`
  в big-endian порядке.
- **`tea_encrypt_block` / `tea_decrypt_block`** — 32 цикла сети Фейстеля
  с константой золотого сечения `TEA_DELTA = 0x9e3779b9`. Принимают
  `const TeaKey*` и указатели на `v0`, `v1`.
- **`pkcs7_pad` / `pkcs7_unpad`** (static) — дополнение до кратности 8 байт
  и снятие паддинга. При ошибке паддинга возвращают пустой вектор
  (не бросают исключений).
- **`pack_block` / `unpack_block`** (static) — упаковка двух `uint32_t`
  в 8 байт и обратно (big-endian).
- **`tea_cbc_encrypt` / `tea_cbc_decrypt`** — режим CBC для данных
  произвольной длины. При ошибке возвращают пустой вектор.

Этот файл **не знает** о консоли, файлах или динамических библиотеках.

### `capi/tea_capi.h`

C-совместимый интерфейс модуля. Объявляет `extern "C"` функции и
непрозрачный указатель `TEAHandle`. Макрос `TEA_API` превращается в
`__declspec(dllexport)` при сборке библиотеки и в `__declspec(dllimport)`
при использовании из `app`.

Функции:
- `tea_create` / `tea_destroy` — создание и уничтожение контекста
- `tea_set_key` — установка ключа
- `tea_encrypt_cbc` / `tea_decrypt_cbc` — шифрование/дешифрование
- `tea_free_buffer` — освобождение буфера результата

### `capi/tea_capi.cpp`

Реализация C-интерфейса. Внутри — `struct TEAContext` с полем `TeaKey key`.
Контекст создаётся через `malloc`, уничтожается через `free` — никакого
C++ рантайма на границе библиотеки.

- **`tea_create`** — `malloc(sizeof(TEAContext))` + `tea_key_init()`
- **`tea_destroy`** — `free(handle)`
- **`tea_set_key`** — вызывает `tea_key_set()`
- **`tea_encrypt_cbc` / `tea_decrypt_cbc`** — вызывают `tea_cbc_encrypt` /
  `tea_cbc_decrypt`. Результат копируется в `malloc`-буфер. При пустом
  результате возвращают `0`.
- **`tea_free_buffer`** — `free(buffer)`

### `ui/tea_module_api.h` / `tea_module_api.cpp`

`struct TEAModule` — хранит `DynamicLibrary library` и шесть указателей на
функции (`create`, `destroy`, `setKey`, `encryptCbc`, `decryptCbc`,
`freeBuffer`). Никакой логики внутри структуры.

Свободные функции:
- **`tea_load(module, path)`** — загружает `tea.dll`/`tea.so` и привязывает
  все символы через `tea_bind_symbol<>`. При неудаче выгружает библиотеку.
- **`tea_is_ready(module)`** — проверяет что все указатели заполнены.
- **`tea_last_error(module)`** — текст последней ошибки.
- **`tea_bind_symbol<FnPtr>(module, name, target)`** — шаблонная функция
  привязки одного символа, реализована в заголовке.

### `ui/tea_utils.h` / `tea_utils.cpp`

Свободные функции с префиксом `tea_` — заменяет namespace, исключает
конфликты линковки при подключении нескольких шифров в одном `.exe`:

- **`tea_hex_to_bytes` / `tea_bytes_to_hex`** — конвертация HEX ↔ байты
- **`tea_random_bytes`** — генерация случайных байт через `mt19937`
- **`tea_read_file` / `tea_write_file`** — бинарное чтение/запись файлов
- **`tea_ensure_dir`** — создание папки (`stat`/`mkdir`, без `<filesystem>`)
- **`tea_extract_filename`** — извлечение имени файла из пути
- **`tea_build_encrypt_path` / `tea_build_decrypt_path`** — пути в папках
  `Encryptfiles/` и `Decryptfiles/` с авто-созданием папок
- **`tea_pack_filename_header` / `tea_unpack_filename_header`** — упаковка
  оригинального имени файла в бинарный заголовок формата
  `[4 байта длина][N байт имя]`
- **`tea_generate_and_save` / `tea_load_from_file`** — генерация/загрузка
  ключа или IV с выводом HEX в консоль

### `ui/tea_ui.cpp`

Точка входа `runTEA()`, вызываемая из `main.cpp`. Не знает о `TeaKey`
напрямую — вся работа через `TEAModule`:

1. Загружает `tea.dll`/`tea.so` через `tea_load(&mod, LIB_PATH)`.
2. Показывает меню: текст / файл / генератор ключей.
3. **Текстовый режим** — шифрует введённый текст, выводит шифротекст
   в HEX; при расшифровке запрашивает HEX шифротекста, IV берёт из файла.
4. **Файловый режим** — шифрует файл в `.bin` формат с встроенными
   метаданными `[заголовок имени][8 байт IV][шифротекст]`, сохраняет в
   `Encryptfiles/`. При расшифровке восстанавливает оригинальное имя
   и сохраняет в `Decryptfiles/`.
5. **Генератор ключей** — создаёт ключ фиксированной длины 16 байт
   (128 бит — TEA не поддерживает переменную длину) и сохраняет в
   `tea_key.bin`.

---

## Сборка

### Windows (MinGW / g++)

```bash
g++ -std=c++17 -shared -o tea.dll TEA/algo/tea.cpp TEA/capi/tea_capi.cpp -ITEA/algo -ITEA/capi -DTEA_BUILD_DLL
g++ -std=c++17 -o app.exe main.cpp core/loader.cpp TEA/ui/tea_ui.cpp TEA/ui/tea_utils.cpp TEA/ui/tea_module_api.cpp -Icore -ITEA/ui -ITEA/capi
```

### Linux (g++)

```bash
g++ -std=c++17 -shared -fPIC -o tea.so TEA/algo/tea.cpp TEA/capi/tea_capi.cpp -ITEA/algo -ITEA/capi
g++ -std=c++17 -o app main.cpp core/loader.cpp TEA/ui/tea_ui.cpp TEA/ui/tea_utils.cpp TEA/ui/tea_module_api.cpp -Icore -ITEA/ui -ITEA/capi -ldl
```

**`tea.dll` / `tea.so` должна лежать рядом с `app.exe` / `app`** при запуске.

### Через CMake (рекомендуется)

```bash
mkdir build && cd build && cmake .. && cmake --build .
```

---

## Параметры алгоритма

| Параметр      | Значение                         |
|---------------|----------------------------------|
| Размер блока  | 64 бита (8 байт)                 |
| Длина ключа   | 128 бит (16 байт, фиксированная) |
| Число раундов | 32 цикла (64 операции Фейстеля)  |
| Режим работы  | CBC (Cipher Block Chaining)      |
| Паддинг       | PKCS#7                           |