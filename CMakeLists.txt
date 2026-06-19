cmake_minimum_required(VERSION 3.15)
project(rgz_kfs LANGUAGES CXX)

# =============================================================================
#  Общие настройки проекта
# =============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Все .exe/.dll/.so складываем в корень папки сборки — удобно запускать
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
#  Модуль TEA — динамическая библиотека (tea.dll на Windows, tea.so на Linux)
#
#  SHARED означает .dll/.so. Внутрь идут только algo/ и capi/ —
#  чистый алгоритм (свободные функции + struct TeaKey) + C-обёртка.
#  Классов нет. UI и main сюда не входят.
# =============================================================================

add_library(tea SHARED
    TEA/algo/tea.cpp
    TEA/capi/tea_capi.cpp
)

target_include_directories(tea PRIVATE
    TEA/algo
    TEA/capi
)

# TEA_BUILD_DLL определён в начале tea_capi.cpp через #define —
# отдельная compile_definition здесь не нужна, но оставляем для совместимости
# с Windows, где символы .dll нужно явно помечать dllexport.
target_compile_definitions(tea PRIVATE TEA_BUILD_DLL)

# Убираем префикс "lib" на Linux, чтобы файл назывался tea.so, а не libtea.so
set_target_properties(tea PROPERTIES PREFIX "")

# =============================================================================
#  Модуль Blowfish — динамическая библиотека (blowfish.dll / blowfish.so)
#
#  Та же логика что и у TEA: внутрь идут только algo/ и capi/.
#  Классов нет. Используются свободные функции bf_* + struct BFKey.
# =============================================================================

add_library(blowfish SHARED
    Blowfish/algo/blowfish.cpp
    Blowfish/capi/blowfish_capi.cpp
)

target_include_directories(blowfish PRIVATE
    Blowfish/algo
    Blowfish/capi
)

# BLOWFISH_BUILD_DLL определён в начале blowfish_capi.cpp через #define
target_compile_definitions(blowfish PRIVATE BLOWFISH_BUILD_DLL)

# Убираем префикс "lib" на Linux — файл должен называться blowfish.so
set_target_properties(blowfish PROPERTIES PREFIX "")

# =============================================================================
#  Главная программа — app.exe / app
#
#  Внутрь идёт main.cpp + весь UI-слой каждого шифра + общий загрузчик
#  динамических библиотек (core/).
#  algo/ и capi/ сюда НЕ входят — они есть только внутри .dll/.so.
# =============================================================================

add_executable(app
    main.cpp
    core/loader.cpp

    TEA/ui/tea_ui.cpp
    TEA/ui/tea_utils.cpp
    TEA/ui/tea_module_api.cpp

    Blowfish/ui/blowfish_ui.cpp
    Blowfish/ui/blowfish_utils.cpp
    Blowfish/ui/blowfish_module_api.cpp
)

target_include_directories(app PRIVATE
    core

    TEA/ui
    TEA/capi          # нужен tea_capi.h для TEAHandle, TEA_BLOCK_BYTES и т.д.

    Blowfish/ui
    Blowfish/capi     # нужен blowfish_capi.h для BlowfishHandle, BLOWFISH_KEY_MIN и т.д.
)

# ── Платформенные зависимости ─────────────────────────────────────────────────

if(WIN32)
    # LoadLibrary/GetProcAddress/FreeLibrary находятся в kernel32,
    # который линкуется по умолчанию — отдельных флагов не требуется.
else()
    # dlopen/dlsym/dlclose требуют явной линковки с libdl на Linux.
    target_link_libraries(app PRIVATE dl)
endif()

# =============================================================================
#  Гарантируем порядок сборки: библиотеки собираются раньше app
# =============================================================================

add_dependencies(app tea)
add_dependencies(app blowfish)

# =============================================================================
#  Сообщение после конфигурации
# =============================================================================

message(STATUS "")
message(STATUS "  rgz_kfs: сборка настроена.")
message(STATUS "  После сборки app (или app.exe), tea.so/.dll и blowfish.so/.dll")
message(STATUS "  окажутся в одной папке: ${CMAKE_BINARY_DIR}")
message(STATUS "")