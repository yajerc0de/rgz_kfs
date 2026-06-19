# Модуль AES (Advanced Encryption Standard, AES-128)

Реализация блочного симметричного шифра AES-128 в виде динамической библиотеки
(`aes.dll` / `aes.so`) с консольным интерфейсом, который загружает её в рантайме.

---

## Структура директорий

```
AES/
├── algo/                       ← чистый алгоритм, попадает ТОЛЬКО в aes.dll/.so
│   ├── aes_tables.h            ← S-Box, Inv S-Box, Rcon
│   ├── aes_tables.cpp
│   ├── aes_core.h              ← константы Nb/Nk/Nr + прототипы свободных функций
│   ├── aes_core.cpp            ← раунды шифра (ShiftRows, SubBytes, MixColumns...)
│   ├── aes_cbc.h                ← прототипы режима CBC
│   └── aes_cbc.cpp             ← реализация CBC поверх encrypt_block/decrypt_block
│
├── capi/                       ← C-обёртка для динамической библиотеки
│   ├── aes_capi.h               ← extern "C" + AESHandle
│   └── aes_capi.cpp            ← struct AESContext + new/delete
│
└── ui/                          ← консольный интерфейс, попадает ТОЛЬКО в app
    ├── aes_module_api.h        ← struct AESModule + aes_bind_symbol<>
    ├── aes_module_api.cpp      ← aes_load / aes_is_ready / aes_last_error
    ├── aes_utils.h              ← утилиты ключей/файлов/HEX
    ├── aes_utils.cpp
    ├── aes_ui.h                 ← AppMode, InputSource, прототипы UI
    └── aes_ui.cpp               ← aes_run_ui()
```

Дополнительно используется общий файл вне папки `AES/`:

```
core/
├── loader.h                    ← кросс-платформенный загрузчик .dll/.so
└── loader.cpp                    (общий для всех шифров, не дублируется)
```

---

## За что отвечает каждый файл

### `algo/aes_tables.h` / `aes_tables.cpp`

Статические таблицы алгоритма, не зависящие от ключа и данных:

- **`s_box`** — таблица замен (S-Box) для `SubBytes`.
- **`inv_s_box`** — обратная таблица замен для `InvSubBytes`.
- **`Rcon`** — раундовые константы для `expand_key`.

Никакой логики — только данные, экспортируемые через `AES_API`.

### `algo/aes_core.h`

Объявляет константы алгоритма уровня файла (`Nb`, `Nk`, `Nr`, `BLOCK_SIZE`,
`ROUND_KEYS_SIZE`) и свободные функции:

- `bytes_to_matrix` / `matrix_to_bytes` — преобразование массива байт в
  State-матрицу 4×4 и обратно.
- `expand_key` — разворачивание ключа в раундовые ключи (Key Expansion).
- `encrypt_block` / `decrypt_block` — шифрование/дешифрование одного блока
  (16 байт).

### `algo/aes_core.cpp`

Реализация алгоритма AES-128 без классов:

- **`add_round_key`** (static) — XOR состояния с раундовым ключом.
- **`sub_bytes` / `inv_sub_bytes`** (static) — замена байт через
  `s_box` / `inv_s_box`.
- **`shift_rows` / `inv_shift_rows`** (static) — циклический сдвиг строк
  State-матрицы.
- **`gf_mul`** (static) — умножение в поле Галуа GF(2⁸) по модулю `0x1B`.
- **`mix_columns` / `inv_mix_columns`** (static) — линейное преобразование
  столбцов State-матрицы через `gf_mul`.
- **`bytes_to_matrix` / `matrix_to_bytes`** — раскладка 16 байт в матрицу
  4×4 по столбцам и обратно.
- **`expand_key`** — первый раундовый ключ = мастер-ключ; далее для каждого
  нового слова при `i % Nk == 0` выполняются `RotWord`, `SubWord` и XOR с
  `Rcon`.
- **`encrypt_block`** — `AddRoundKey` + (`Nr - 1`) полных раундов
  (`SubBytes` → `ShiftRows` → `MixColumns` → `AddRoundKey`) + финальный
  раунд без `MixColumns`.
- **`decrypt_block`** — те же шаги в обратном порядке и с обратными
  преобразованиями (`InvShiftRows`, `InvSubBytes`, `InvMixColumns`).

Этот файл **не знает** о консоли, файлах, режиме CBC или динамических
библиотеках — только блочный шифр на 16 байт.

### `algo/aes_cbc.h` / `aes_cbc.cpp`

Режим CBC для данных произвольной длины поверх `encrypt_block` /
`decrypt_block`:

- **`cbc_encrypt`** — дополняет данные PKCS#7 до кратности `BLOCK_SIZE`,
  затем для каждого блока выполняет XOR с предыдущим шифроблоком (на
  первом блоке — с `iv`) и шифрует его.
- **`cbc_decrypt`** — дешифрует каждый блок и снимает XOR с предыдущим
  шифроблоком, в конце удаляет PKCS#7-паддинг по последнему байту
  результата.

Файл не выполняет проверку корректности паддинга сверх диапазона
`1..BLOCK_SIZE` — при повреждённых данных поведение не детектируется
отдельно.

### `capi/aes_capi.h`

C-совместимый интерфейс модуля. Объявляет `extern "C"` функции и
непрозрачный указатель `AESHandle`. Макрос `AES_API` превращается в
`__declspec(dllexport)` при сборке библиотеки и в `__declspec(dllimport)`
при использовании из `app` (на Linux/macOS — `visibility("default")`).

Функции:
- `aes_create` / `aes_destroy` — создание и уничтожение контекста
- `aes_set_key` — установка ключа (разворачивание в раундовые ключи)
- `aes_encrypt_cbc` / `aes_decrypt_cbc` — шифрование/дешифрование
- `aes_free_buffer` — освобождение буфера результата

### `capi/aes_capi.cpp`

Реализация C-интерфейса. Внутри — `struct AESContext` с полем
`unsigned char roundKeys[ROUND_KEYS_SIZE]` и флагом `keyReady`. В отличие
от чисто-C-вариантов, контекст создаётся через `new` и уничтожается через
`delete` (граница библиотеки всё равно остаётся C-совместимой за счёт
`extern "C"` и непрозрачного `AESHandle`).

- **`aes_create`** — `new AESContext`, `roundKeys` обнуляется через
  `memset`, `keyReady = false`.
- **`aes_destroy`** — `delete` по указателю, приведённому к `AESContext*`.
- **`aes_set_key`** — проверяет длину ключа (`AES_KEY_BYTES == 16`),
  вызывает `expand_key()`, выставляет `keyReady = true`.
- **`aes_encrypt_cbc` / `aes_decrypt_cbc`** — проверяют входные указатели и
  `keyReady`, вызывают `cbc_encrypt` / `cbc_decrypt`, копируют результат в
  `malloc`-буфер через `copyVectorToMallocBuffer`. При пустом или
  неудачном выделении возвращают `0`.
- **`aes_free_buffer`** — `free(buffer)`.

### `ui/aes_module_api.h` / `aes_module_api.cpp`

`struct AESModule` — хранит `DynamicLibrary library` и шесть указателей на
функции (`create`, `destroy`, `setKey`, `encryptCbc`, `decryptCbc`,
`freeBuffer`). Никакой логики внутри структуры.

Свободные функции:
- **`aes_load(module, path)`** — загружает `aes.dll`/`aes.so` и привязывает
  все символы через `aes_bind_symbol<>`. При неудаче выгружает библиотеку.
- **`aes_is_ready(module)`** — проверяет, что все указатели заполнены.
- **`aes_last_error(module)`** — текст последней ошибки.
- **`aes_bind_symbol<FnPtr>(module, name, target)`** — шаблонная функция
  привязки одного символа, реализована в заголовке.

### `ui/aes_utils.h` / `aes_utils.cpp`

Свободные функции без префикса (название модуля задаётся файлом, а не
именами функций):

- **`generate_and_save_key`** — генерация случайного 16-байтного ключа
  через `mt19937` и сохранение в бинарный файл, с выводом HEX в консоль.
- **`load_key`** — загрузка 16-байтного ключа из файла.
- **`generate_iv`** — генерация случайного 16-байтного IV через
  `random_device`.
- **`save_encrypted_file`** — сохраняет `[16 байт IV][1 байт длина
  расширения][N байт расширение][шифротекст]` в один файл.
- **`load_encrypted_file`** — разбирает файл того же формата обратно на
  IV, расширение и шифротекст.
- **`read_binary_file` / `write_binary_file`** — бинарное чтение/запись
  произвольных файлов.
- **`print_hex`** — вывод массива байт в HEX-формате в консоль.

В отличие от схемы с упаковкой полного имени файла, здесь в
зашифрованный `.bin` пишется только **расширение** исходного файла —
само имя не сохраняется и при дешифровании восстанавливается как
`<имя_зашифрованного_файла>_decrypted.<расширение>`.

### `ui/aes_ui.h` / `aes_ui.cpp`

Точка входа `aes_run_ui()`, вызываемая из `main.cpp`. Не знает о
`AESContext` напрямую — вся работа через `AESModule` и глобальный
`AESHandle g_handle`.

1. Загружает `aes.dll`/`aes.so`/`aes.dylib` через `aes_load(&g_aes, libName)`
   и создаёт контекст через `g_aes.create()`.
2. Показывает меню: шифрование / дешифрование / выход
   (`aes_show_main_menu`).
3. **Шифрование** (`aes_run_encrypt_mode`) — запрашивает источник данных
   (`ask_input_source`: текст из консоли или файл с диска).
   - Для текста: расширение фиксируется как `txt`, результат сохраняется
     в `encrypted_text.bin`.
   - Для файла: расширение извлекается из пути (`get_extension`),
     результат сохраняется как `<имя_без_расширения>_encrypted.bin`.
   - Ключ генерируется автоматически на каждое шифрование
     (`generate_and_save_key` → `key.bin`), отдельного пункта меню
     "генератор ключей" нет. IV генерируется отдельно через
     `generate_iv` и не сохраняется в `key.bin`.
4. **Дешифрование** (`aes_run_decrypt_mode`) — запрашивает путь к
   зашифрованному файлу и путь к файлу ключа, восстанавливает IV и
   расширение из самого `.bin`-файла, результат сохраняется как
   `<имя_без_расширения>_decrypted.<расширение>`.
5. Все файлы (ключ, зашифрованные/расшифрованные данные) сохраняются в
   текущую рабочую директорию (`./`) — отдельных папок
   `Encryptfiles/`/`Decryptfiles/` модуль не создаёт.

---

## Сборка

### Windows (MinGW / g++)

```bash
g++ -std=c++17 -shared -o aes.dll AES/algo/aes_tables.cpp AES/algo/aes_core.cpp AES/algo/aes_cbc.cpp AES/capi/aes_capi.cpp -IAES/algo -IAES/capi -DAES_BUILD_DLL
g++ -std=c++17 -o app.exe main.cpp core/loader.cpp AES/ui/aes_ui.cpp AES/ui/aes_utils.cpp AES/ui/aes_module_api.cpp -Icore -IAES/ui -IAES/capi
```

### Linux (g++)

```bash
g++ -std=c++17 -shared -fPIC -o aes.so AES/algo/aes_tables.cpp AES/algo/aes_core.cpp AES/algo/aes_cbc.cpp AES/capi/aes_capi.cpp -IAES/algo -IAES/capi
g++ -std=c++17 -o app main.cpp core/loader.cpp AES/ui/aes_ui.cpp AES/ui/aes_utils.cpp AES/ui/aes_module_api.cpp -Icore -IAES/ui -IAES/capi -ldl
```

**`aes.dll` / `aes.so` (или `aes.dylib` на macOS) должна лежать рядом с
`app.exe` / `app`** при запуске.

### Через CMake (рекомендуется)

```bash
mkdir build && cd build && cmake .. && cmake --build .
```

---

## Параметры алгоритма

| Параметр      | Значение                         |
|---------------|-----------------------------------|
| Размер блока  | 128 бит (16 байт)                |
| Длина ключа   | 128 бит (16 байт, AES-128)       |
| Число раундов | 10 (`Nr`)                         |
| Режим работы  | CBC (Cipher Block Chaining)      |
| Паддинг       | PKCS#7                            |
