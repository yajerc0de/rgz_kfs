# Модуль RC5

Реализация блочного симметричного шифра RC5-32/12/16 в виде динамической
библиотеки (`rc5.dll` / `rc5.so`) с консольным интерфейсом, который загружает
её в рантайме.

Алгоритм реализован через свободные функции и структуру `Rc5KeySchedule`.
Утилиты UI имеют префикс `rc5_` для исключения конфликтов имён при линковке
в одном `.exe` с другими модулями.

---

## Структура директорий

```
RC5/
├── algo/                       ← чистый алгоритм, попадает ТОЛЬКО в rc5.dll/.so
│   ├── rc5.h                   ← константы + struct Rc5KeySchedule + прототипы
│   └── rc5.cpp                 ← реализация алгоритма RC5
│
├── capi/                       ← C-обёртка для динамической библиотеки
│   ├── rc5_capi.h              ← extern "C" + Rc5Handle
│   └── rc5_capi.cpp            ← struct Rc5Context + malloc/free
│
└── ui/                         ← консольный интерфейс, попадает ТОЛЬКО в app
    ├── rc5_module_api.h        ← struct Rc5Module + указатели на функции
    ├── rc5_module_api.cpp      ← rc5ModuleLoad / rc5ModuleIsReady
    ├── rc5_utils.h             ← утилиты с префиксом rc5_
    ├── rc5_utils.cpp
    └── rc5_ui.cpp              ← runRC5()
```

---

## За что отвечает каждый файл

### `algo/rc5.h`

Объявляет константы и структуру состояния ключа:

- `RC5_ROUNDS = 12` — число раундов
- `RC5_BLOCK_LEN = 8` — размер блока в байтах (64 бита, два слова по 32 бита)
- `RC5_TABLE_SIZE = 2 * (RC5_ROUNDS + 1) = 26` — размер таблицы подключей S
- `RC5_P32 = 0xB7E15163` и `RC5_Q32 = 0x9E3779B9` — магические константы,
  производные от числа e и золотого сечения φ
- `struct Rc5KeySchedule` — хранит массив подключей `S[26]` и флаг `keyIsSet`

### `algo/rc5.cpp`

Реализация алгоритма RC5-32/12/16 через свободные функции:

- **`rc5InitSchedule`** — сбрасывает структуру в начальное состояние.
- **`rc5SetKey`** — процедура расширения ключа (Key Schedule): преобразует
  16-байтный ключ в массив из 26 подключей `S[0]...S[25]`. Использует
  константы P32 и Q32, три прохода смешивания (mixing).
- **`rc5EncryptBlock`** — один блок: два слова A и B проходят 12 раундов,
  в каждом из которых применяются операции сложения (+), XOR (⊕) и
  циклического вращения влево (<<<) на переменное число позиций.
  Формула раунда:
  `A = ((A ⊕ B) <<< B) + S[2i]`, `B = ((B ⊕ A) <<< A) + S[2i+1]`.
- **`rc5DecryptBlock`** — обратный обход тех же раундов в обратном порядке
  с использованием вращения вправо (>>>).
- **`rc5Pkcs7Pad` / `rc5Pkcs7Unpad`** (static) — паддинг PKCS#7 до кратности
  8 байтам и его снятие.
- **`rc5PackBlock` / `rc5UnpackBlock`** (static) — сериализация двух `uint32_t`
  в 8 байт и обратно в формате **little-endian** (отличие от AES/Blowfish,
  где big-endian).
- **`rc5EncryptCBC` / `rc5DecryptCBC`** — режим CBC для данных произвольной
  длины. IV передаётся как `vector<uint8_t>` длиной 8 байт.

### `capi/rc5_capi.h`

C-совместимый интерфейс. Непрозрачный дескриптор `Rc5Handle`,
макрос `RC5_API` (`__declspec(dllexport/import)` / `visibility("default")`).

Макросы: `RC5_BLOCK_BYTES = 8`, `RC5_KEY_BYTES = 16`.

Функции: `rc5_create`, `rc5_destroy`, `rc5_set_key`,
`rc5_encrypt_cbc`, `rc5_decrypt_cbc`, `rc5_free_buffer`.

### `capi/rc5_capi.cpp`

Внутренняя структура `Rc5Context` содержит `Rc5KeySchedule sched`.
Контекст создаётся через `malloc`, уничтожается через `free`. Результаты
шифрования/дешифрования копируются в `malloc`-буфер и возвращаются
вызывающей стороне; освобождение — через `rc5_free_buffer`.

### `ui/rc5_module_api.h` / `rc5_module_api.cpp`

`struct Rc5Module` — хранит `DynamicLibrary lib` и шесть указателей на
функции (`create`, `destroy`, `setKey`, `encryptCbc`, `decryptCbc`,
`freeBuffer`).

- **`rc5ModuleLoad(mod, path)`** — загружает `rc5.dll`/`rc5.so` и привязывает
  все символы.
- **`rc5ModuleIsReady(mod)`** — проверяет что все указатели заполнены.

### `ui/rc5_utils.h` / `rc5_utils.cpp`

Свободные функции с префиксом `rc5_` — исключают конфликты имён при
линковке, поскольку Speck содержит функции с теми же именами без префикса:

- `rc5_hexToBytes` / `rc5_bytesToHex`
- `rc5_randomBytes`
- `rc5_readFile` / `rc5_writeFile` / `rc5_ensureDir`
- `rc5_extractFilename` / `rc5_buildEncryptPath` / `rc5_buildDecryptPath`
- `rc5_packFilenameHeader` / `rc5_unpackFilenameHeader`
- `rc5_generateAndSave` / `rc5_loadFromFile`

### `ui/rc5_ui.cpp`

Точка входа `runRC5()`, вызываемая из `main.cpp`. Загружает `rc5.dll`/`rc5.so`
через `rc5ModuleLoad`. Три режима: текст, файл, генератор ключа.
Формат зашифрованного файла: `[заголовок имени][8 байт IV][шифротекст]`.

---

## Сборка

### Через CMake (рекомендуется)

```bash
mkdir build && cd build && cmake .. -G "MinGW Makefiles" && cmake --build .
```

### Windows (g++ вручную)

```bash
g++ -std=c++17 -shared -o rc5.dll RC5/algo/rc5.cpp RC5/capi/rc5_capi.cpp -IRC5/algo -IRC5/capi -DRC5_BUILD_DLL
g++ -std=c++17 -o app.exe main.cpp core/loader.cpp RC5/ui/rc5_ui.cpp RC5/ui/rc5_utils.cpp RC5/ui/rc5_module_api.cpp -Icore -IRC5/ui -IRC5/capi
```

### Linux (g++ вручную)

```bash
g++ -std=c++17 -shared -fPIC -o rc5.so RC5/algo/rc5.cpp RC5/capi/rc5_capi.cpp -IRC5/algo -IRC5/capi
g++ -std=c++17 -o app main.cpp core/loader.cpp RC5/ui/rc5_ui.cpp RC5/ui/rc5_utils.cpp RC5/ui/rc5_module_api.cpp -Icore -IRC5/ui -IRC5/capi -ldl
```

---

## Параметры алгоритма

| Параметр      | Значение                              |
|---------------|---------------------------------------|
| Вариант       | RC5-32/12/16                          |
| Размер слова  | 32 бита                               |
| Размер блока  | 64 бита (8 байт, два слова по 32 бита)|
| Длина ключа   | 128 бит (16 байт, фиксированная)      |
| Число раундов | 12                                    |
| Режим работы  | CBC (Cipher Block Chaining)           |
| Паддинг       | PKCS#7                                |
| Порядок байт  | Little-endian                         |