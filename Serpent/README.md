# Модуль Serpent

Реализация блочного симметричного шифра Serpent в виде динамической библиотеки
(`serpent.dll` / `serpent.so`) с консольным интерфейсом, который загружает её в рантайме.

---

## Структура директорий

```
Serpent/
├── algo/                           ← чистый алгоритм, попадает ТОЛЬКО в serpent.dll/.so
│   ├── serpent_tables.h            ← S-боксы (прямые и обратные)
│   ├── serpent_tables.cpp
│   ├── serpent_core.h              ← константы + прототипы свободных функций
│   ├── serpent_core.cpp            ← раунды шифра (apply_sbox, linear_transform...)
│   ├── serpent_cbc.h               ← прототипы режима CBC
│   └── serpent_cbc.cpp             ← реализация CBC поверх serpent_encrypt_block/decrypt_block
│
├── capi/                           ← C-обёртка для динамической библиотеки
│   ├── serpent_capi.h              ← extern "C" + SerpentHandle
│   └── serpent_capi.cpp            ← struct SerpentContext + new/delete
│
└── ui/                             ← консольный интерфейс, попадает ТОЛЬКО в app
    ├── serpent_module_api.h        ← struct SerpentModule + serpent_bind_symbol<>
    ├── serpent_module_api.cpp      ← serpent_load / serpent_is_ready / serpent_last_error
    ├── serpent_utils.h             ← утилиты ключей/файлов/HEX
    ├── serpent_utils.cpp
    ├── serpent_ui.h                ← SerpentAppMode, SerpentInputSource, прототипы UI
    └── serpent_ui.cpp              ← serpent_run_ui()
```

Дополнительно используется общий файл вне папки `Serpent/`:

```
core/
├── loader.h                    ← кросс-платформенный загрузчик .dll/.so
└── loader.cpp                    (общий для всех шифров, не дублируется)
```

---

## За что отвечает каждый файл

### `algo/serpent_tables.h` / `serpent_tables.cpp`

Статические таблицы алгоритма, не зависящие от ключа и данных:

- **`serpent_sbox[8][16]`** — 8 прямых S-боксов из официальной спецификации Serpent. Каждый
  S-бокс применяется в раундах с номерами `i % 8 == box_index` (раунд `i` использует
  бокс с индексом `i % 8`).
- **`serpent_sbox_inv[8][16]`** — 8 обратных S-боксов для дешифрования. Для каждого
  бокса `S` выполняется: если `S[x] = y`, то `S_inv[y] = x`.

Никакой логики — только данные, экспортируемые через `SERPENT_API`.

### `algo/serpent_core.h`

Объявляет константы алгоритма (`BLOCK_SIZE = 16`, `KEY_SIZE = 16`,
`NUM_ROUNDS = 32`, `SUBKEYS_COUNT = 132`) и три свободные функции:

- `serpent_expand_key` — разворачивание 128-битного ключа в 132 подключа по 32 бита.
- `serpent_encrypt_block` / `serpent_decrypt_block` — шифрование/дешифрование одного
  блока (16 байт).

### `algo/serpent_core.cpp`

Реализация алгоритма Serpent без классов:

- **`rotate_left` / `rotate_right`** (static) — циклические сдвиги 32-битного слова.
- **`apply_sbox`** (static) — применяет один из 8 S-боксов к четырём 32-битным словам
  `(x0, x1, x2, x3)` побитово: для каждого из 32 бит собирает тетраду из соответствующих
  битов четырёх слов, прогоняет через `serpent_sbox` / `serpent_sbox_inv` и раскладывает
  результат обратно. Флаг `inverse` переключает между прямым и обратным боксом.
- **`linear_transform` / `inverse_linear_transform`** (static) — линейное преобразование
  над четырьмя словами состояния, применяемое после S-бокса в раундах 0–30. Включает
  серию циклических сдвигов и XOR операций с фиксированными константами.
- **`xor_with_subkey`** (static) — XOR состояния `(x0, x1, x2, x3)` с четырьмя
  32-битными словами подключа с номером `key_index`.
- **`bytes_to_words` / `words_to_bytes`** (static) — преобразование 16-байтного блока
  в четыре слова `uint32_t` в little-endian порядке и обратно.
- **`serpent_expand_key`** — Key Schedule: входной 128-битный ключ расширяется до массива
  `w[8 + 132]` через линейный рекуррентный сдвиговый регистр с константой `PHI = 0x9E3779B9`
  и циклическим сдвигом на 11 бит. Затем 33 группы по 4 слова прогоняются через S-боксы
  в порядке `(35 - i) % 8`, формируя итоговый массив из 132 подключей.
- **`serpent_encrypt_block`** — 32 одинаковых раунда: `XorWithSubKey` → `apply_sbox`
  (прямой) → `linear_transform` (не применяется в последнем раунде 31). После всех
  раундов — финальный `XorWithSubKey` с подключом № 32.
- **`serpent_decrypt_block`** — зеркальная последовательность: начальный
  `XorWithSubKey` с подключом № 32, затем 32 раунда в обратном порядке:
  `inverse_linear_transform` (не применяется для раунда 31) → `apply_sbox` (обратный)
  → `XorWithSubKey`.

Этот файл **не знает** о консоли, файлах, режиме CBC или динамических
библиотеках — только блочный шифр на 16 байт.

### `algo/serpent_cbc.h` / `serpent_cbc.cpp`

Режим CBC для данных произвольной длины поверх `serpent_encrypt_block` /
`serpent_decrypt_block`:

- **`serpent_cbc_encrypt`** — добавляет PKCS#7 паддинг до кратности `BLOCK_SIZE`,
  затем для каждого блока выполняет XOR с предыдущим шифроблоком (на первом
  блоке — с `iv`) и шифрует его.
- **`serpent_cbc_decrypt`** — дешифрует каждый блок и снимает XOR с предыдущим
  шифроблоком, в конце удаляет PKCS#7-паддинг по значению последнего байта результата.

Файл не выполняет дополнительную проверку корректности паддинга сверх диапазона
`1..BLOCK_SIZE` — при повреждённых данных некорректный паддинг не детектируется отдельно.

### `capi/serpent_capi.h`

C-совместимый интерфейс модуля. Объявляет `extern "C"` функции и непрозрачный
указатель `SerpentHandle`. Макрос `SERPENT_API` превращается в
`__declspec(dllexport)` при сборке библиотеки и в `__declspec(dllimport)` при
использовании из `app` (на Linux/macOS — `visibility("default")`).

Константы: `SERPENT_BLOCK_BYTES = 16`, `SERPENT_KEY_BYTES = 16`.

Функции:
- `serpent_create` / `serpent_destroy` — создание и уничтожение контекста
- `serpent_set_key` — установка ключа (разворачивание в 132 подключа)
- `serpent_encrypt_cbc` / `serpent_decrypt_cbc` — шифрование/дешифрование
- `serpent_free_buffer` — освобождение буфера результата

### `capi/serpent_capi.cpp`

Реализация C-интерфейса. Внутри — `struct SerpentContext` с полем
`unsigned int subkeys[SUBKEYS_COUNT]` и флагом `keyReady`. Контекст создаётся
через `new` и уничтожается через `delete` (граница библиотеки остаётся C-совместимой
за счёт `extern "C"` и непрозрачного `SerpentHandle`).

- **`serpent_create`** — `new SerpentContext`, `subkeys` обнуляется через `memset`,
  `keyReady = false`.
- **`serpent_destroy`** — `delete` по указателю, приведённому к `SerpentContext*`.
- **`serpent_set_key`** — проверяет длину ключа (`SERPENT_KEY_BYTES == 16`), вызывает
  `serpent_expand_key()`, выставляет `keyReady = true`.
- **`serpent_encrypt_cbc` / `serpent_decrypt_cbc`** — проверяют входные указатели и
  `keyReady`, вызывают `serpent_cbc_encrypt` / `serpent_cbc_decrypt`, копируют
  результат в `malloc`-буфер через `copyVectorToMallocBuffer`. При пустом или
  неудачном выделении возвращают `0`.
- **`serpent_free_buffer`** — `free(buffer)`.

### `ui/serpent_module_api.h` / `serpent_module_api.cpp`

`struct SerpentModule` — хранит `DynamicLibrary library` и шесть указателей на
функции (`create`, `destroy`, `setKey`, `encryptCbc`, `decryptCbc`, `freeBuffer`).
Никакой логики внутри структуры.

Свободные функции:
- **`serpent_load(module, path)`** — загружает `serpent.dll`/`serpent.so` и привязывает
  все символы через `serpent_bind_symbol<>`. При неудаче выгружает библиотеку.
- **`serpent_is_ready(module)`** — проверяет, что все шесть указателей заполнены.
- **`serpent_last_error(module)`** — текст последней ошибки.
- **`serpent_bind_symbol<FnPtr>(module, name, target)`** — шаблонная функция привязки
  одного символа, реализована в заголовке.

### `ui/serpent_utils.h` / `serpent_utils.cpp`

Свободные функции с префиксом `serpent_` — исключают конфликты линковки при
подключении нескольких шифров в одном `.exe`:

- **`serpent_generate_and_save_key`** — генерация случайного 16-байтного ключа через
  `mt19937` и сохранение в бинарный файл, с выводом HEX в консоль.
- **`serpent_load_key`** — загрузка 16-байтного ключа из файла.
- **`serpent_generate_iv`** — генерация случайного 16-байтного IV через `random_device`.
- **`serpent_save_encrypted_file`** — сохраняет `[16 байт IV][1 байт длина расширения]
  [N байт расширение][шифротекст]` в один файл.
- **`serpent_load_encrypted_file`** — разбирает файл того же формата обратно на IV,
  расширение и шифротекст.
- **`serpent_read_binary_file` / `serpent_write_binary_file`** — бинарное
  чтение/запись произвольных файлов.
- **`serpent_print_hex`** — вывод массива байт в HEX-формате в консоль.
- **`serpent_get_extension`** — возвращает расширение файла без точки.
- **`serpent_strip_extension`** — возвращает имя файла без расширения.

В зашифрованный `.bin` записывается только **расширение** исходного файла —
само имя не сохраняется и при дешифровании восстанавливается как
`<имя_зашифрованного_файла>_decrypted.<расширение>`.

### `ui/serpent_ui.h` / `serpent_ui.cpp`

Точка входа `serpent_run_ui()`, вызываемая из `main.cpp`. Не знает о
`SerpentContext` напрямую — вся работа через `SerpentModule` и глобальный
`SerpentHandle g_handle`.

1. Загружает `serpent.dll`/`serpent.so`/`libserpent.dylib` через
   `serpent_load(&g_serpent, libName)` и создаёт контекст через `g_serpent.create()`.
2. Показывает меню: шифрование / дешифрование / выход (`serpent_show_main_menu`).
3. **Шифрование** (`serpent_run_encrypt_mode`) — запрашивает источник данных
   (`ask_input_source`: текст из консоли или файл с диска).
   - Для текста: расширение фиксируется как `txt`, результат сохраняется в
     `encrypted_text.bin`.
   - Для файла: расширение извлекается из пути (`serpent_get_extension`),
     результат сохраняется как `<имя_без_расширения>_encrypted.bin`.
   - Ключ генерируется автоматически на каждое шифрование
     (`serpent_generate_and_save_key` → `key.bin`). IV генерируется отдельно через
     `serpent_generate_iv` и не сохраняется в `key.bin` — он встраивается в заголовок
     зашифрованного файла.
4. **Дешифрование** (`serpent_run_decrypt_mode`) — запрашивает путь к зашифрованному
   файлу и путь к файлу ключа, восстанавливает IV и расширение из самого
   `.bin`-файла, результат сохраняется как
   `<имя_без_расширения>_decrypted.<расширение>`.
5. Все файлы (ключ, зашифрованные/расшифрованные данные) сохраняются в текущую
   рабочую директорию (`./`) — отдельных папок `Encryptfiles/`/`Decryptfiles/`
   модуль не создаёт.

---

## Сборка

### Windows (MinGW / g++)

```bash
g++ -std=c++17 -shared -o serpent.dll Serpent/algo/serpent_tables.cpp Serpent/algo/serpent_core.cpp Serpent/algo/serpent_cbc.cpp Serpent/capi/serpent_capi.cpp -ISerpent/algo -ISerpent/capi -DSERPENT_BUILD_DLL
g++ -std=c++17 -o app.exe main.cpp core/loader.cpp Serpent/ui/serpent_ui.cpp Serpent/ui/serpent_utils.cpp Serpent/ui/serpent_module_api.cpp -Icore -ISerpent/ui -ISerpent/capi
```

### Linux (g++)

```bash
g++ -std=c++17 -shared -fPIC -o serpent.so Serpent/algo/serpent_tables.cpp Serpent/algo/serpent_core.cpp Serpent/algo/serpent_cbc.cpp Serpent/capi/serpent_capi.cpp -ISerpent/algo -ISerpent/capi
g++ -std=c++17 -o app main.cpp core/loader.cpp Serpent/ui/serpent_ui.cpp Serpent/ui/serpent_utils.cpp Serpent/ui/serpent_module_api.cpp -Icore -ISerpent/ui -ISerpent/capi -ldl
```

**`serpent.dll` / `serpent.so` (или `libserpent.dylib` на macOS) должна лежать рядом с
`app.exe` / `app`** при запуске.

### Через CMake (рекомендуется)

```bash
mkdir build && cd build && cmake .. && cmake --build .
```

---

## Параметры алгоритма

| Параметр       | Значение                            |
|----------------|-------------------------------------|
| Размер блока   | 128 бит (16 байт)                   |
| Длина ключа    | 128 бит (16 байт)                   |
| Число раундов  | 32                                  |
| Количество подключей | 132 (по 32 бита каждый)       |
| S-боксы        | 8 штук по 16 значений, применяются циклически |
| Режим работы   | CBC (Cipher Block Chaining)         |
| Паддинг        | PKCS#7                              |
