# Модуль TEA (Tiny Encryption Algorithm)

Реализация блочного симметричного шифра TEA в виде динамической библиотеки
(`tea.dll` / `tea.so`) с консольным интерфейсом, который загружает её в рантайме.

---

## Структура директорий

```
TEA/
├── algo/                       ← чистый алгоритм, попадает ТОЛЬКО в tea.dll/.so
│   ├── tea.h
│   └── tea.cpp
│
├── capi/                       ← C-обёртка для динамической библиотеки
│   ├── tea_capi.h
│   └── tea_capi.cpp
│
└── ui/                         ← консольный интерфейс, попадает ТОЛЬКО в app.exe
    ├── tea_module_api.h
    ├── tea_module_api.cpp
    ├── tea_utils.h
    ├── tea_utils.cpp
    └── tea_ui.cpp
```

Дополнительно используется общий файл вне папки `TEA/`:

```
core/
├── module_loader.h              ← кросс-платформенный загрузчик .dll/.so
└── module_loader.cpp              (общий для всех шифров, не дублируется)
```

---

## За что отвечает каждый файл

### `algo/tea.h`

Объявление класса `TEA`. Описывает константы алгоритма (`DELTA`, `ROUNDS`,
`KEY_BYTES`, `BLOCK_BYTES`), публичные методы (`setKey`, `encryptBlock`,
`decryptBlock`, `encryptCBC`, `decryptCBC`) и приватное состояние
(массив из 4 слов ключа). Не содержит логики — только интерфейс.

### `algo/tea.cpp`

Реализация самого алгоритма TEA:

- **`setKey`** — разбивает 16-байтный ключ на 4 слова `uint32_t`.
- **`encryptBlock` / `decryptBlock`** — 32 цикла сети Фейстеля с константой
  золотого сечения `DELTA = 0x9e3779b9`.
- **`pkcs7Pad` / `pkcs7Unpad`** — дополнение данных до кратности 8 байт и
  снятие паддинга при расшифровке.
- **`encryptCBC` / `decryptCBC`** — режим сцепления блоков (Cipher Block
  Chaining): шифрует данные произвольной длины, используя IV и предыдущий
  блок шифротекста.

Этот файл **не знает** о консоли, файлах или динамических библиотеках —
чистая криптография.

### `capi/tea_capi.h`

C-совместимый интерфейс модуля. Поскольку классы C++ нельзя безопасно
передавать через границу `.dll`/`.so` (несовместимость ABI разных
компиляторов), здесь объявлены только функции `extern "C"` и непрозрачный
указатель `TeaHandle` (скрывает `TEA*` от внешнего мира). Также объявляет
макрос `TEA_API`, который превращается в `__declspec(dllexport)` при сборке
библиотеки и в `__declspec(dllimport)` при использовании из `app.exe`.

### `capi/tea_capi.cpp`

Реализация C-интерфейса — тонкая обёртка над классом `TEA`:

- **`tea_create` / `tea_destroy`** — создание и удаление объекта `TEA`
  через `new`/`delete`, спрятанные за `TeaHandle`.
- **`tea_set_key`** — обёртка над `TEA::setKey`.
- **`tea_encrypt_cbc` / `tea_decrypt_cbc`** — обёртки над `encryptCBC`/
  `decryptCBC`. Результат копируется в буфер, выделенный через `malloc`
  (а не `new`), так как `.exe` и `.dll` могут использовать разные кучи.
  Все исключения C++ перехватываются внутри — наружу библиотеки они
  никогда не вылетают.
- **`tea_free_buffer`** — освобождает буфер, выделенный предыдущими
  функциями. Обязателен к вызову на стороне `app.exe`.

### `ui/tea_module_api.h` / `tea_module_api.cpp`

Класс `TeaModule` — обёртка над `DynamicLibrary` (см. `core/module_loader.h`)
специально для TEA. При вызове `load("tea.dll")`:

1. Загружает саму библиотеку через `DynamicLibrary::load`.
2. Через `GetProcAddress`/`dlsym` находит все шесть функций из
   `tea_capi.h` (`tea_create`, `tea_destroy`, `tea_set_key`,
   `tea_encrypt_cbc`, `tea_decrypt_cbc`, `tea_free_buffer`).
3. Сохраняет их как указатели на функции — доступны как
   `module.create()`, `module.encryptCbc(...)` и т.д.

Если хотя бы один символ не найден — `load()` возвращает `false`
(несовместимая или повреждённая библиотека).

### `ui/tea_utils.h` / `tea_utils.cpp`

Набор утилит, не зависящих от алгоритма шифрования:

- **`hexToBytes` / `bytesToHex`** — конвертация между HEX-строкой и байтами.
- **`randomBytes`** — генерация случайных байт через `mt19937`.
- **`readFile` / `writeFile`** — бинарное чтение/запись файлов.
- **`ensureDir`** — создание папки, если она не существует
  (`stat`/`mkdir`, без `<filesystem>` для совместимости с MinGW).
- **`extractFilename`** — извлечение имени файла из пути.
- **`buildEncryptPath` / `buildDecryptPath`** — построение путей в папках
  `Encryptfiles/` и `Decryptfiles/` (с авто-созданием папок).
- **`packFilenameHeader` / `unpackFilenameHeader`** — упаковка оригинального
  имени файла в бинарный заголовок внутри `.bin` файла и обратное извлечение.
  Формат: `[4 байта длина имени][N байт имя файла]`.
- **`generateAndSave` / `loadFromFile`** — генерация ключа/IV с сохранением
  в файл, либо загрузка существующего, с выводом HEX в консоль.

### `ui/tea_ui.cpp`

Точка входа `runTEA()`, вызываемая из `main.cpp`. Не содержит **никакого**
прямого упоминания класса `TEA` — вся работа идёт через `TeaModule`:

1. Загружает `tea.dll`/`tea.so` через `TeaModule::load`.
2. Показывает меню: текст / файл / генератор ключей.
3. **Текстовый режим** — шифрует введённый текст, выводит шифротекст и IV
   в HEX; при расшифровке запрашивает HEX шифротекста, IV берёт из файла.
4. **Файловый режим** — шифрует файл в формат `.bin` с встроенными
   метаданными (оригинальное имя + IV + шифротекст), сохраняет в
   `Encryptfiles/`. При расшифровке восстанавливает оригинальное имя
   и расширение, сохраняет в `Decryptfiles/`.
5. **Генератор ключей** — создаёт ключ фиксированной длины (16 байт,
   128 бит — TEA не поддерживает переменную длину ключа) и сохраняет
   в `tea_key.bin`.

---

## Сборка

### Windows (MinGW / g++)

```bash
g++ -std=c++17 -shared -o tea.dll TEA/algo/tea.cpp TEA/capi/tea_capi.cpp -ITEA/algo -ITEA/capi
g++ -std=c++17 -o app.exe main.cpp core/module_loader.cpp TEA/ui/tea_ui.cpp TEA/ui/tea_utils.cpp TEA/ui/tea_module_api.cpp -Icore -ITEA/ui -ITEA/capi
```

### Linux (g++)

```bash
g++ -std=c++17 -shared -fPIC -o tea.so TEA/algo/tea.cpp TEA/capi/tea_capi.cpp -ITEA/algo -ITEA/capi
g++ -std=c++17 -o app main.cpp core/module_loader.cpp TEA/ui/tea_ui.cpp TEA/ui/tea_utils.cpp TEA/ui/tea_module_api.cpp -Icore -ITEA/ui -ITEA/capi -ldl
```

**`tea.dll` / `tea.so` должна лежать рядом с `app.exe` / `app`** при запуске —
иначе `runTEA()` выведет ошибку загрузки библиотеки.

### Через CMake (рекомендуется, собирает весь проект целиком)

Из корня репозитория, где лежит `CMakeLists.txt`:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

CMake сам подставит правильные флаги (`-shared`/`-fPIC` на Linux,
`__declspec` на Windows) и соберёт и `tea.dll`/`tea.so`, и `app.exe`/`app`
в одну папку `build/` — собирать модули по отдельности не нужно.

---

## Параметры алгоритма

| Параметр       | Значение                          |
|----------------|------------------------------------|
| Размер блока   | 64 бита (8 байт)                   |
| Длина ключа    | 128 бит (16 байт, фиксированная)   |
| Число раундов  | 32 цикла (64 операции Фейстеля)    |
| Режим работы   | CBC (Cipher Block Chaining)        |
| Паддинг        | PKCS#7                             |