# Модуль Speck

Реализация блочного симметричного шифра Speck128/128 в виде динамической
библиотеки (`speck.dll` / `speck.so`) с консольным интерфейсом, который
загружает её в рантайме.

Алгоритм реализован через свободные функции и структуру `SpeckKeySchedule`.
Утилиты UI имеют префикс `speck_` для исключения конфликтов имён при линковке
в одном `.exe` с другими модулями (в первую очередь с RC5, утилиты которого
имели идентичные имена).

---

## Структура директорий

```
Speck/
├── algo/                       ← чистый алгоритм, попадает ТОЛЬКО в speck.dll/.so
│   ├── speck.h                 ← константы + struct SpeckKeySchedule + прототипы
│   └── speck.cpp               ← реализация алгоритма Speck
│
├── capi/                       ← C-обёртка для динамической библиотеки
│   ├── speck_capi.h            ← extern "C" + SpeckHandle
│   └── speck_capi.cpp          ← struct SpeckContext + malloc/free
│
└── ui/                         ← консольный интерфейс, попадает ТОЛЬКО в app
    ├── speck_module_api.h      ← struct SpeckModule + указатели на функции
    ├── speck_module_api.cpp    ← speckModuleLoad / speckModuleIsReady
    ├── speck_utils.h           ← утилиты с префиксом speck_
    ├── spec_utils.cpp          ← реализация утилит (имя файла оставлено как в оригинале)
    └── speck_ui.cpp            ← runSpeck()
```

---

## За что отвечает каждый файл

### `algo/speck.h`

Объявляет константы и структуру состояния ключа:

- `SPECK_BLOCK_BYTES = 16` — размер блока 128 бит (два слова `uint64_t`)
- `struct SpeckKeySchedule` — хранит массив подключей `K[34]`, число раундов
  `rounds` (32, 33 или 34 в зависимости от длины ключа) и флаг `keyIsSet`

### `algo/speck.cpp`

Реализация Speck128 через свободные функции:

- **`speckInitSchedule`** — сбрасывает структуру в нулевое состояние.
- **`speckSetKey`** — процедура расширения ключа. Поддерживает ключи длиной
  128, 192 и 256 бит (16, 24, 32 байта). Число раундов:
  128 бит → 32 раунда, 192 бит → 33 раунда, 256 бит → 34 раунда.
  Алгоритм использует ARX-структуру (сложение, вращение, XOR):
  `L[i+m-1] = (K[i] + rotr(L[i], α)) ⊕ i`, `K[i+1] = rotl(K[i], β) ⊕ L[i+m-1]`,
  где α = 8, β = 3 для 64-битных слов.
- **`speckEncryptBlock`** — один 128-битный блок (слова x и y) шифруется
  за `rounds` итераций по формулам:
  `x = (rotr(x, 8) + y) ⊕ k`, `y = rotl(y, 3) ⊕ x`,
  где k — подключ текущего раунда из `K[]`.
- **`speckDecryptBlock`** — обратный обход раундов:
  `y = rotr(x ⊕ y, 3)`, `x = rotl((x ⊕ k) - y, 8)`.
- **`pkcs7Pad` / `pkcs7Unpad`** (static) — паддинг PKCS#7 до кратности
  16 байтам. При ошибке бросает `std::runtime_error`.
- **`packBlock` / `unpackBlock`** (static) — сериализация двух `uint64_t`
  в 16 байт и обратно в формате **little-endian**.
- **`speckEncryptCBC` / `speckDecryptCBC`** — режим CBC для данных
  произвольной длины. IV передаётся как `vector<uint8_t>` длиной 16 байт.

### `capi/speck_capi.h`

C-совместимый интерфейс. Непрозрачный дескриптор `SpeckHandle`,
макрос `SPECK_API` (`__declspec(dllexport/import)` / `visibility("default")`).

Макрос `SPECK_CAPI_BLOCK_BYTES = 16` (переименован из `SPECK_BLOCK_BYTES`
чтобы не конфликтовать с `constexpr SPECK_BLOCK_BYTES` из `speck.h`).

Функции: `speck_create`, `speck_destroy`, `speck_set_key`,
`speck_encrypt_cbc`, `speck_decrypt_cbc`, `speck_free_buffer`.

### `capi/speck_capi.cpp`

Внутренняя структура `SpeckContext` содержит `SpeckKeySchedule sched`.
Контекст создаётся через `malloc`, уничтожается через `free`. Результаты
копируются в `malloc`-буфер; освобождение — через `speck_free_buffer`.

### `ui/speck_module_api.h` / `speck_module_api.cpp`

`struct SpeckModule` — хранит `DynamicLibrary lib` и шесть указателей на
функции (`create`, `destroy`, `setKey`, `encryptCbc`, `decryptCbc`,
`freeBuffer`).

- **`speckModuleLoad(mod, path)`** — загружает `speck.dll`/`speck.so` и
  привязывает все символы.
- **`speckModuleIsReady(mod)`** — проверяет что все указатели заполнены.

### `ui/speck_utils.h` / `spec_utils.cpp`

Свободные функции с префиксом `speck_` — исключают конфликты имён при
линковке, поскольку RC5 содержит утилиты с теми же именами без префикса:

- `speck_hexToBytes` / `speck_bytesToHex`
- `speck_randomBytes`
- `speck_readFile` / `speck_writeFile` / `speck_ensureDir`
- `speck_extractFilename` / `speck_buildEncryptPath` / `speck_buildDecryptPath`
- `speck_packFilenameHeader` / `speck_unpackFilenameHeader`
- `speck_generateAndSave` / `speck_loadFromFile`

Примечание: файл реализации называется `spec_utils.cpp` (без буквы `k`) —
это оригинальное имя файла из репозитория, сохранено намеренно для
совместимости с историей Git.

### `ui/speck_ui.cpp`

Точка входа `runSpeck()`, вызываемая из `main.cpp`. Оригинальный файл содержал
`int main()` — переименован в `void runSpeck()` для интеграции в общее
приложение. Загружает `speck.dll`/`speck.so` через `speckModuleLoad`. Три
режима: текст, файл, генератор ключа.

---

## Сборка

### Через CMake (рекомендуется)

```bash
mkdir build && cd build && cmake .. -G "MinGW Makefiles" && cmake --build .
```

### Windows (g++ вручную)

```bash
g++ -std=c++17 -shared -o speck.dll Speck/algo/speck.cpp Speck/capi/speck_capi.cpp -ISpeck/algo -ISpeck/capi -DSPECK_BUILD_DLL
g++ -std=c++17 -o app.exe main.cpp core/loader.cpp Speck/ui/speck_ui.cpp Speck/ui/spec_utils.cpp Speck/ui/speck_module_api.cpp -Icore -ISpeck/ui -ISpeck/capi
```

### Linux (g++ вручную)

```bash
g++ -std=c++17 -shared -fPIC -o speck.so Speck/algo/speck.cpp Speck/capi/speck_capi.cpp -ISpeck/algo -ISpeck/capi
g++ -std=c++17 -o app main.cpp core/loader.cpp Speck/ui/speck_ui.cpp Speck/ui/spec_utils.cpp Speck/ui/speck_module_api.cpp -Icore -ISpeck/ui -ISpeck/capi -ldl
```

---

## Параметры алгоритма

| Параметр      | Значение                                        |
|---------------|-------------------------------------------------|
| Вариант       | Speck128/128 (основной), 128/192, 128/256       |
| Размер блока  | 128 бит (16 байт, два слова по 64 бита)         |
| Длина ключа   | 128, 192 или 256 бит (16 / 24 / 32 байта)       |
| Число раундов | 32 (для 128-бит ключа)                          |
| Структура     | ARX (сложение, вращение, XOR)                   |
| Вращения      | α = 8 (вправо), β = 3 (влево) для 64-бит слов  |
| Режим работы  | CBC (Cipher Block Chaining)                     |
| Паддинг       | PKCS#7                                          |
| Порядок байт  | Little-endian                                   |