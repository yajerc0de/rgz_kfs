#pragma once

#include <string>

using namespace std;



class DynamicLibrary {
public:
    DynamicLibrary() = default;
    ~DynamicLibrary();

   
    DynamicLibrary(const DynamicLibrary&)            = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;

    
    bool load(const string& path);

   
    void unload();

   
    void* getSymbol(const string& symbolName) const;

    
    bool isLoaded() const;

  
    const string& lastError() const;

private:
    void*  m_handle = nullptr;   
    string m_lastError;
};