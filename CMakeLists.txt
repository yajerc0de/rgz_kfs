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

add_executable(app
    main.cpp
    core/loader.cpp

    TEA/ui/tea_ui.cpp
    TEA/ui/tea_utils.cpp
    TEA/ui/tea_module_api.cpp


)

target_include_directories(app PRIVATE
    core
    TEA/ui       TEA/capi
)

if(WIN32)
else()
    target_link_libraries(app PRIVATE dl)
endif()

add_dependencies(app tea)

message(STATUS "")
message(STATUS "  rgz_kfs: AES | Serpent | RC5 | Speck | Blowfish | TEA")
message(STATUS "  Выходная папка: ${CMAKE_BINARY_DIR}")
message(STATUS "")
