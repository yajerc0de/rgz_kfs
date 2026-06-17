#pragma once

// Кросс-платформенные макросы экспорта/импорта символов динамической библиотеки
#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef SERPENT_BUILDING_LIB
        #define SERPENT_API __declspec(dllexport)
    #else
        #define SERPENT_API __declspec(dllimport)
    #endif
#else
    #ifdef SERPENT_BUILDING_LIB
        #define SERPENT_API __attribute__((visibility("default")))
    #else
        #define SERPENT_API
    #endif
#endif
