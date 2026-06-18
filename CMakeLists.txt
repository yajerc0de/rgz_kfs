cmake_minimum_required(VERSION 3.15)
project(rgz_kfs LANGUAGES CXX)

# =============================================================================
#  Общие настройки проекта
# =============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
#  Модуль TEA — динамическая библиотека
# =============================================================================

add_library(tea SHARED
    TEA/algo/tea.cpp
    TEA/capi/tea_capi.cpp
)

target_include_directories(tea PRIVATE
    TEA/algo
    TEA/capi
)

target_compile_definitions(tea PRIVATE TEA_BUILD_DLL)
set_target_properties(tea PROPERTIES PREFIX "")

# =============================================================================
#  Модуль RC5 — динамическая библиотека
# =============================================================================

add_library(rc5 SHARED
    RC5/algo/rc5.cpp
    RC5/capi/rc5_capi.cpp
)

target_include_directories(rc5 PRIVATE
    RC5/algo
    RC5/capi
)

target_compile_definitions(rc5 PRIVATE RC5_BUILD_DLL)
set_target_properties(rc5 PROPERTIES PREFIX "")

# =============================================================================
#  Исполняемый файл приложения
# =============================================================================

add_executable(app
    main.cpp
    core/loader.cpp

    TEA/ui/tea_ui.cpp
    TEA/ui/tea_utils.cpp
    TEA/ui/tea_module_api.cpp

    RC5/ui/rc5_ui.cpp
    RC5/ui/rc5_utils.cpp
    RC5/ui/rc5_module_api.cpp
)

target_include_directories(app PRIVATE
    core
    TEA/ui
    TEA/capi
    RC5/ui
    RC5/capi
)

if(WIN32)
else()
    target_link_libraries(app PRIVATE dl)
endif()

add_dependencies(app tea rc5)

message(STATUS "")
message(STATUS "  rgz_kfs: сборка настроена.")
message(STATUS "  После сборки app, tea.so и rc5.so")
message(STATUS "  окажутся в одной папке: ${CMAKE_BINARY_DIR}")
message(STATUS "")
