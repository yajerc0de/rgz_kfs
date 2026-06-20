#ifdef _WIN32
    // Эти макросы должны быть определены ДО включения <windows.h>:
    //  - WIN32_LEAN_AND_MEAN убирает редко используемые части windows.h
    //    (в т.ч. часть COM-заголовков, которые конфликтуют с std::byte)
    //  - NOMINMAX отключает макросы min/max, которые иначе ломают <algorithm>
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

#include "loader.h"

using namespace std;

// =============================================================================
//  Деструктор — гарантирует выгрузку библиотеки даже если пользователь забыл
//  вызвать unload() явно.
// =============================================================================

DynamicLibrary::~DynamicLibrary() {
    unload();
}

// =============================================================================
//  load — загрузка библиотеки
// =============================================================================

bool DynamicLibrary::load(const string& path) {
    // Если уже что-то загружено — сначала выгружаем
    unload();

#ifdef _WIN32
    m_handle = static_cast<void*>(LoadLibraryA(path.c_str()));

    if (m_handle == nullptr) {
        DWORD errCode = GetLastError();
        m_lastError = "LoadLibrary не удалось, код ошибки Windows: "
                    + to_string(errCode);
        return false;
    }
#else
    m_handle = dlopen(path.c_str(), RTLD_NOW);

    if (m_handle == nullptr) {
        const char* err = dlerror();
        m_lastError = "dlopen не удалось: " + string(err ? err : "неизвестная ошибка");
        return false;
    }
#endif

    m_lastError.clear();
    return true;
}

// =============================================================================
//  unload — выгрузка библиотеки
// =============================================================================

void DynamicLibrary::unload() {
    if (m_handle == nullptr) return;

#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(m_handle));
#else
    dlclose(m_handle);
#endif

    m_handle = nullptr;
}

// =============================================================================
//  getSymbol — получение указателя на функцию по имени
// =============================================================================

void* DynamicLibrary::getSymbol(const string& symbolName) const {
    if (m_handle == nullptr) return nullptr;

#ifdef _WIN32
    return reinterpret_cast<void*>(
        GetProcAddress(static_cast<HMODULE>(m_handle), symbolName.c_str())
    );
#else
    // dlerror() нужно сбросить перед вызовом, чтобы корректно отличить
    // "символ не найден" от "символ существует но равен nullptr" —
    // в нашем случае это не критично, просто возвращаем результат dlsym.
    return dlsym(m_handle, symbolName.c_str());
#endif
}

// =============================================================================
//  Вспомогательные методы
// =============================================================================

bool DynamicLibrary::isLoaded() const {
    return m_handle != nullptr;
}

const string& DynamicLibrary::lastError() const {
    return m_lastError;
}