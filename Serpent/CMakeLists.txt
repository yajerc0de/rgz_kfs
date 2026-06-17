cmake_minimum_required(VERSION 3.15)
project(SERPENT_CBC_Project CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Единая папка для всех бинарников (.exe/.dylib/.so/.dll),
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# Динамическая библиотека: вся AES/CBC логика + утилиты
add_library(serpent_crypto SHARED
    lib/serpent_tables.cpp
    lib/serpent_core.cpp
    lib/serpent_cbc.cpp
    lib/utils.cpp
)

target_include_directories(serpent_crypto PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Гарантируем, что AES_BUILDING_LIB определён на уровне компиляции
target_compile_definitions(serpent_crypto PRIVATE SERPENT_BUILDING_LIB)

set_target_properties(serpent_crypto PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    OUTPUT_NAME "serpent_crypto"          # -> libaes_crypto.so / .dylib / aes_crypto.dll
    WINDOWS_EXPORT_ALL_SYMBOLS OFF    # экспортируем только то, что помечено AES_API
)

# Исполняемый файл (меню + main), линкуется с aes_crypto динамически
add_executable(serpent_app
    app/main.cpp
    app/menu.cpp
)

target_include_directories(serpent_app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/app
)

target_link_libraries(serpent_app PRIVATE serpent_crypto)

if(APPLE)
    set_target_properties(serpent_app PROPERTIES
        BUILD_RPATH "@loader_path"
        INSTALL_RPATH "@loader_path"
    )
elseif(UNIX)
    set_target_properties(serpent_app PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()