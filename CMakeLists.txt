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
#  чистый алгоритм + C-обёртка. UI и main сюда не входят.
# =============================================================================

add_library(tea SHARED
    TEA/algo/tea.cpp
    TEA/capi/tea_capi.cpp
)

target_include_directories(tea PRIVATE
    TEA/algo
    TEA/capi
)

# tea_capi.h сам активирует __declspec(dllexport) через TEA_BUILD_DLL,
# но определение TEA_BUILD_DLL должно прийти именно при сборке ЭТОЙ библиотеки
target_compile_definitions(tea PRIVATE TEA_BUILD_DLL)

# Убираем префикс "lib" на Linux, чтобы файл назывался tea.so, а не libtea.so —
# это нужно, потому что наш загрузчик ищет файл с именем "tea" без префикса
set_target_properties(tea PROPERTIES PREFIX "")

add_executable(app
    main.cpp
    core/module_loader.cpp

    TEA/ui/tea_ui.cpp
    TEA/ui/tea_utils.cpp
    TEA/ui/tea_module_api.cpp

)

target_include_directories(app PRIVATE
    core
    TEA/ui
    TEA/capi      # нужен tea_capi.h для типов TeaHandle, TEA_BLOCK_BYTES и т.д.

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
#  Гарантируем порядок сборки: библиотеки собираются раньше app —
#  хотя на этапе компиляции они не зависят друг от друга (только в рантайме),
#  это удобно для ясности и чтобы tea.dll точно лежала в папке сборки
#  к моменту, когда app.exe будет готов для первого запуска.
# =============================================================================

add_dependencies(app tea)

# =============================================================================
#  Сообщение после конфигурации
# =============================================================================

message(STATUS "")
message(STATUS "  rgz_kfs: сборка настроена.")
message(STATUS "  После сборки app.exe (или app) и tea.dll (или tea.so)")
message(STATUS "  окажутся в одной папке: ${CMAKE_BINARY_DIR}")
message(STATUS "")