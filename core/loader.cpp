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



DynamicLibrary::~DynamicLibrary() {
    unload();
}



bool DynamicLibrary::load(const string& path) {
    
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



void DynamicLibrary::unload() {
    if (m_handle == nullptr) return;

#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(m_handle));
#else
    dlclose(m_handle);
#endif

    m_handle = nullptr;
}



void* DynamicLibrary::getSymbol(const string& symbolName) const {
    if (m_handle == nullptr) return nullptr;

#ifdef _WIN32
    return reinterpret_cast<void*>(
        GetProcAddress(static_cast<HMODULE>(m_handle), symbolName.c_str())
    );
#else
    
    return dlsym(m_handle, symbolName.c_str());
#endif
}



bool DynamicLibrary::isLoaded() const {
    return m_handle != nullptr;
}

const string& DynamicLibrary::lastError() const {
    return m_lastError;
}