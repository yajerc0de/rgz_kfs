#pragma once

// Кросс-платформенные макросы экспорта/импорта символов динамической библиотеки
#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef AES_BUILDING_LIB
        #define AES_API __declspec(dllexport)
    #else
        #define AES_API __declspec(dllimport)
    #endif
#else
    #ifdef AES_BUILDING_LIB
        #define AES_API __attribute__((visibility("default")))
    #else
        #define AES_API
    #endif
#endif
