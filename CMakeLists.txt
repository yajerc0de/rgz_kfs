cmake_minimum_required(VERSION 3.15)
project(rgz_kfs_rc5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

macro(strip_lib_prefix target)
    set_target_properties(${target} PROPERTIES PREFIX "")
endmacro()

# =============================================================================
#  Модуль RC5 — динамическая библиотека
# =============================================================================

add_library(rc5 SHARED
    RC5/algo/rc5.cpp
    RC5/capi/rc5_capi.cpp
)
target_include_directories(rc5 PRIVATE RC5/algo RC5/capi)
target_compile_definitions(rc5 PRIVATE RC5_BUILD_DLL)
strip_lib_prefix(rc5)

# =============================================================================
#  Главная программа
# =============================================================================

add_executable(app
    main.cpp
    core/loader.cpp

    RC5/ui/rc5_ui.cpp
    RC5/ui/rc5_utils.cpp
    RC5/ui/rc5_module_api.cpp
)

target_include_directories(app PRIVATE
    core
    RC5/ui
    RC5/capi
)

if(WIN32)
else()
    target_link_libraries(app PRIVATE dl)
endif()

add_dependencies(app rc5)

message(STATUS "")
message(STATUS "  rgz_kfs RC5dop: rc5.dll + app.exe")
message(STATUS "  Выходная папка: ${CMAKE_BINARY_DIR}")
message(STATUS "")
