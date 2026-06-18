# Модуль Blowfish

Реализация блочного симметричного шифра Blowfish в виде динамической
библиотеки (`blowfish.dll` / `blowfish.so`) с консольным интерфейсом,
который загружает её в рантайме.

---

## Структура директорий

```
Blowfish/
├── algo/                       ← чистый алгоритм, попадает ТОЛЬКО в blowfish.dll/.so
│   ├── blowfish.h
│   └── blowfish.cpp
│
├── capi/                       ← C-обёртка для динамической библиотеки
│   ├── blowfish_capi.h
│   └── blowfish_capi.cpp
│
└── ui/                         ← консольный интерфейс, попадает ТОЛЬКО в app.exe
    ├── blowfish_module_api.h
    ├── blowfish_module_api.cpp
    ├── blowfish_utils.h
    ├── blowfish_utils.cpp
    └── blowfish_ui.cpp
```

Дополнительно используется общий файл вне папки `Blowfish/`:

```
core/
├── module_loader.h              ← кросс-платформенный загрузчик .dll/.so
└── module_loader.cpp              (общий для всех шифров, не дублируется)
```

---

## За что отвечает каждый файл

### `algo/blowfish.h`

Объявление класса `Blowfish`. Описывает константы алгоритма (`ROUNDS`,
`P_ARRAY_SIZE`, `S_BOX_COUNT`, `S_BOX_SIZE`, `BLOCK_BYTES`, `KEY_MIN`,
`KEY_MAX`), публичные методы (`setKey`, `encryptBlock`, `decryptBlock`,
`encryptCBC`, `decryptCBC`) и приватное состояние (P-массив из 18 слов,
4 S-блока по 256 слов каждый). Не содержит логики — только интерфейс.

### `algo/blowfish.cpp`

Реализация самого алгоритма Blowfish:

- **Константы инициализации** — дробные части числа π, используемые для
  заполнения P-массива и S-блоков перед расширением ключа (~4 КБ таблиц).
- **`setKey` (Key Schedule)** — трёхэтапное расширение ключа: сброс P/S к
  константам π, XOR P-массива с байтами ключа, затем 521 шифрование
  нулевого блока для генерации финальных P и S.
- **Функция `F(x)`** — нелинейное преобразование через 4 S-блока:
  `((S[0][a] + S[1][b]) ^ S[2][c]) + S[3][d]`.
- **`encryptBlock` / `decryptBlock`** — 16 раундов сети Фейстеля.
- **`pkcs7Pad` / `pkcs7Unpad`** — дополнение данных до кратности 8 байт и
  снятие паддинга при расшифровке.
- **`encryptCBC` / `decryptCBC`** — режим сцепления блоков (Cipher Block
  Chaining) для данных произвольной длины.

Этот файл **не знает** о консоли, файлах или динамических библиотеках —
чистая криптография.

### `capi/blowfish_capi.h`

C-совместимый интерфейс модуля. Поскольку классы C++ нельзя безопасно
передавать через границу `.dll`/`.so` (несовместимость ABI разных
компиляторов), здесь объявлены только функции `extern "C"` и непрозрачный
указатель `BlowfishHandle` (скрывает `Blowfish*` от внешнего мира). Также
объявляет макрос `BLOWFISH_API`, который превращается в
`__declspec(dllexport)` при сборке библиотеки и в `__declspec(dllimport)`
при использовании из `app.exe`.

### `capi/blowfish_capi.cpp`

Реализация C-интерфейса — тонкая обёртка над классом `Blowfish`:

- **`blowfish_create` / `blowfish_destroy`** — создание и удаление объекта
  `Blowfish` через `new`/`delete`, спрятанные за `BlowfishHandle`.
- **`blowfish_set_key`** — обёртка над `Blowfish::setKey`, запускает
  Key Schedule внутри библиотеки.
- **`blowfish_encrypt_cbc` / `blowfish_decrypt_cbc`** — обёртки над
  `encryptCBC`/`decryptCBC`. Результат копируется в буфер, выделенный
  через `malloc` (а не `new`), так как `.exe` и `.dll` могут использовать
  разные кучи. Все исключения C++ перехватываются внутри — наружу
  библиотеки они никогда не вылетают.
- **`blowfish_free_buffer`** — освобождает буфер, выделенный предыдущими
  функциями. Обязателен к вызову на стороне `app.exe`.

### `ui/blowfish_module_api.h` / `blowfish_module_api.cpp`

Класс `BlowfishModule` — обёртка над `DynamicLibrary` (см.
`core/module_loader.h`) специально для Blowfish. При вызове
`load("blowfish.dll")`:

1. Загружает саму библиотеку через `DynamicLibrary::load`.
2. Через `GetProcAddress`/`dlsym` находит все шесть функций из
   `blowfish_capi.h` (`blowfish_create`, `blowfish_destroy`,
   `blowfish_set_key`, `blowfish_encrypt_cbc`, `blowfish_decrypt_cbc`,
   `blowfish_free_buffer`).
3. Сохраняет их как указатели на функции — доступны как
   `module.create()`, `module.encryptCbc(...)` и т.д.

Если хотя бы один символ не найден — `load()` возвращает `false`
(несовместимая или повреждённая библиотека).

### `ui/blowfish_utils.h` / `blowfish_utils.cpp`

Набор утилит, не зависящих от алгоритма шифрования:

- **`hexToBytes` / `bytesToHex`** — конвертация между HEX-строкой и байтами.
- **`randomBytes`** — генерация случайных байт через `mt19937`.
- **`readFile` / `writeFile`** — бинарное чтение/запись файлов.
- **`ensureDir`** — создание папки, если она не существует
  (`stat`/`mkdir`, без `<filesystem>` для совместимости с MinGW).
- **`extractFilename` / `extractExtension`** — извлечение имени файла и
  расширения из пути.
- **`buildEncryptPath` / `buildDecryptPath`** — построение путей в папках
  `Encryptfiles/` и `Decryptfiles/` (с авто-созданием папок).
- **`packFilenameHeader` / `unpackFilenameHeader`** — упаковка оригинального
  имени файла в бинарный заголовок внутри `.bin` файла и обратное извлечение.
  Формат: `[4 байта длина имени][N байт имя файла]`.
- **`generateAndSave` / `loadFromFile`** — генерация ключа/IV с сохранением
  в файл, либо загрузка существующего, с выводом HEX в консоль.

### `ui/blowfish_ui.cpp`

Точка входа `runBlowfish()`, вызываемая из `main.cpp`. Не содержит
**никакого** прямого упоминания класса `Blowfish` — вся работа идёт через
`BlowfishModule`:

1. Загружает `blowfish.dll`/`blowfish.so` через `BlowfishModule::load`.
2. Показывает меню: текст / файл / генератор ключей.
3. **Текстовый режим** — шифрует введённый текст, выводит шифротекст и IV
   в HEX; при расшифровке запрашивает HEX шифротекста, IV берёт из файла.
4. **Файловый режим** — шифрует файл в формат `.bin` с встроенными
   метаданными (оригинальное имя + IV + шифротекст), сохраняет в
   `Encryptfiles/`. При расшифровке восстанавливает оригинальное имя
   и расширение, сохраняет в `Decryptfiles/`.
5. **Генератор ключей** — запрашивает у пользователя длину ключа от
   `BLOWFISH_KEY_MIN` (4 байта) до `BLOWFISH_KEY_MAX` (56 байт) — в отличие
   от TEA, Blowfish поддерживает переменную длину ключа — и сохраняет
   в `blowfish_key.bin`.

Также содержит `inputKey()` — вспомогательную функцию для ручного ввода
ключа в HEX-формате с проверкой допустимой длины.

---

## Сборка

### Windows (MinGW / g++)

```bash
g++ -std=c++17 -shared -o blowfish.dll Blowfish/algo/blowfish.cpp Blowfish/capi/blowfish_capi.cpp -IBlowfish/algo -IBlowfish/capi
g++ -std=c++17 -o app.exe main.cpp core/module_loader.cpp Blowfish/ui/blowfish_ui.cpp Blowfish/ui/blowfish_utils.cpp Blowfish/ui/blowfish_module_api.cpp -Icore -IBlowfish/ui -IBlowfish/capi
```

### Linux (g++)

```bash
g++ -std=c++17 -shared -fPIC -o blowfish.so Blowfish/algo/blowfish.cpp Blowfish/capi/blowfish_capi.cpp -IBlowfish/algo -IBlowfish/capi
g++ -std=c++17 -o app main.cpp core/module_loader.cpp Blowfish/ui/blowfish_ui.cpp Blowfish/ui/blowfish_utils.cpp Blowfish/ui/blowfish_module_api.cpp -Icore -IBlowfish/ui -IBlowfish/capi -ldl
```

**`blowfish.dll` / `blowfish.so` должна лежать рядом с `app.exe` / `app`**
при запуске — иначе `runBlowfish()` выведет ошибку загрузки библиотеки.

### Через CMake (рекомендуется, собирает весь проект целиком)

Из корня репозитория, где лежит `CMakeLists.txt`:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

CMake сам подставит правильные флаги (`-shared`/`-fPIC` на Linux,
`__declspec` на Windows) и соберёт и `blowfish.dll`/`blowfish.so`, и
`app.exe`/`app` в одну папку `build/` — собирать модули по отдельности
не нужно.

---

## Параметры алгоритма

| Параметр       | Значение                            |
|----------------|--------------------------------------|
| Размер блока   | 64 бита (8 байт)                     |
| Длина ключа    | 32–448 бит (4–56 байт, переменная)   |
| Число раундов  | 16                                    |
| Режим работы   | CBC (Cipher Block Chaining)           |
| Паддинг        | PKCS#7                                |