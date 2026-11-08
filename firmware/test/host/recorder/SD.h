#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <sys/stat.h>
#include <direct.h>

#define FILE_READ  "rb"
#define FILE_WRITE "wb"

class File {
public:
    FILE* f = nullptr;
    File() = default;
    explicit File(FILE* p) : f(p) {}
    operator bool() const { return f != nullptr; }
    size_t write(const void* p, size_t n) { return f ? fwrite(p, 1, n, f) : 0; }
    void close() { if (f) { fclose(f); f = nullptr; } }
};

class SDClass {
public:
    std::string real(const char* path) const
    {
        if (path && path[0] == '/') return std::string("test_out") + path;
        return path ? path : "";
    }

    File open(const char* path, const char* mode = "rb")
    {
        return File(fopen(real(path).c_str(), mode));
    }

    bool mkdir(const char* path)
    {
        std::string p = real(path);
        _mkdir(p.c_str());
        return true;
    }

    bool exists(const char* path)
    {
        FILE* f = fopen(real(path).c_str(), "rb");
        if (!f) return false;
        fclose(f);
        return true;
    }
};

extern SDClass SD;
