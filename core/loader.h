#pragma once

#include <string>

using namespace std;

// =============================================================================
//  module_loader.h — кросс-платформенная загрузка динамических библиотек
//
//  Скрывает разницу между Windows (LoadLibrary/GetProcAddress/FreeLibrary)
//  и Linux/Mac (dlopen/dlsym/dlclose) за одним простым интерфейсом.
// =============================================================================

class DynamicLibrary {
public:
    DynamicLibrary() = default;
    ~DynamicLibrary();

    // Запрещаем копирование — у нас один handle на одну загруженную библиотеку.
    // Случайное копирование привело бы к двойному FreeLibrary/dlclose.
    DynamicLibrary(const DynamicLibrary&)            = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;

    // Загрузить библиотеку по пути.
    // На Windows ожидается "tea.dll", на Linux — "libtea.so" или "./tea.so".
    // Возвращает false при ошибке (см. lastError()).
    bool load(const string& path);

    // Выгрузить библиотеку, если она была загружена. Вызывается и в деструкторе.
    void unload();

    // Получить указатель на функцию по имени.
    // Возвращает nullptr если библиотека не загружена или символ не найден.
    void* getSymbol(const string& symbolName) const;

    // Загружена ли библиотека в данный момент.
    bool isLoaded() const;

    // Текст последней ошибки (для вывода пользователю).
    const string& lastError() const;

private:
    void*  m_handle = nullptr;   // HMODULE на Windows, void* (от dlopen) на Linux
    string m_lastError;
};