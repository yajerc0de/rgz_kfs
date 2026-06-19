cmake_minimum_required(VERSION 3.15)
project(rgz_kfs LANGUAGES CXX)

# =============================================================================
#  Общие настройки проекта
# =============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

macro(strip_lib_prefix target)
    set_target_properties(${target} PROPERTIES PREFIX "")
endmacro()

# =============================================================================
#  Модуль TEA
# =============================================================================

add_library(tea SHARED
    TEA/algo/tea.cpp
    TEA/capi/tea_capi.cpp
)
target_include_directories(tea PRIVATE TEA/algo TEA/capi)
target_compile_definitions(tea PRIVATE TEA_BUILD_DLL)
strip_lib_prefix(tea)

# =============================================================================
#  Модуль Blowfish
# =============================================================================

add_library(blowfish SHARED
    Blowfish/algo/blowfish.cpp
    Blowfish/capi/blowfish_capi.cpp
)
target_include_directories(blowfish PRIVATE Blowfish/algo Blowfish/capi)
target_compile_definitions(blowfish PRIVATE BLOWFISH_BUILD_DLL)
strip_lib_prefix(blowfish)

# =============================================================================
#  Модуль AES
# =============================================================================

add_library(aes SHARED
    AES/algo/aes_core.cpp
    AES/algo/aes_cbc.cpp
    AES/algo/aes_tables.cpp
    AES/capi/aes_capi.cpp
)
target_include_directories(aes PRIVATE AES/algo AES/capi)
target_compile_definitions(aes PRIVATE AES_BUILD_DLL)
strip_lib_prefix(aes)

# =============================================================================
#  Модуль Serpent
# =============================================================================

add_library(serpent SHARED
    Serpent/algo/serpent_core.cpp
    Serpent/algo/serpent_cbc.cpp
    Serpent/algo/serpent_tables.cpp
    Serpent/capi/serpent_capi.cpp
)
target_include_directories(serpent PRIVATE Serpent/algo Serpent/capi)
target_compile_definitions(serpent PRIVATE SERPENT_BUILD_DLL)
strip_lib_prefix(serpent)

# =============================================================================
#  Модуль RC5
# =============================================================================

add_library(rc5 SHARED
    RC5/algo/rc5.cpp
    RC5/capi/rc5_capi.cpp
)
target_include_directories(rc5 PRIVATE RC5/algo RC5/capi)
target_compile_definitions(rc5 PRIVATE RC5_BUILD_DLL)
strip_lib_prefix(rc5)

# =============================================================================
#  Модуль Speck
# =============================================================================

add_library(speck SHARED
    Speck/algo/speck.cpp
    Speck/capi/speck_capi.cpp
)
target_include_directories(speck PRIVATE Speck/algo Speck/capi)
target_compile_definitions(speck PRIVATE SPECK_BUILD_DLL)
strip_lib_prefix(speck)

# =============================================================================
#  Главная программа
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

    AES/ui/aes_ui.cpp
    AES/ui/aes_utils.cpp
    AES/ui/aes_module_api.cpp

    Serpent/ui/serpent_ui.cpp
    Serpent/ui/serpent_utils.cpp
    Serpent/ui/serpent_module_api.cpp

    RC5/ui/rc5_ui.cpp
    RC5/ui/rc5_utils.cpp
    RC5/ui/rc5_module_api.cpp

    Speck/ui/speck_ui.cpp
    Speck/ui/spec_utils.cpp
    Speck/ui/speck_module_api.cpp
)

target_include_directories(app PRIVATE
    core
    TEA/ui       TEA/capi
    Blowfish/ui  Blowfish/capi
    AES/ui       AES/capi      AES/algo
    Serpent/ui   Serpent/capi
    RC5/ui       RC5/capi
    Speck/ui     Speck/capi
)

if(WIN32)
else()
    target_link_libraries(app PRIVATE dl)
endif()

add_dependencies(app tea blowfish aes serpent rc5 speck)

message(STATUS "")
message(STATUS "  rgz_kfs: AES | Serpent | RC5 | Speck | Blowfish | TEA")
message(STATUS "  Выходная папка: ${CMAKE_BINARY_DIR}")
message(STATUS "")
