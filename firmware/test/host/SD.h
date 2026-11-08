#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#define FILE_READ "rb"

class File {
public:
    FILE* f = nullptr;
    File() = default;
    explicit File(FILE* p) : f(p) {}
    operator bool() const { return f != nullptr; }

    int read(uint8_t* buf, size_t n)
    {
        return f ? (int)fread(buf, 1, n, f) : -1;
    }

    size_t size() const
    {
        if (!f) return 0;
        long p = ftell(f);
        fseek(f, 0, SEEK_END);
        long s = ftell(f);
        fseek(f, p, SEEK_SET);
        return (size_t)s;
    }

    void close()
    {
        if (f) {
            fclose(f);
            f = nullptr;
        }
    }
};

class SDClass {
public:
    File open(const char* path, const char* mode = "rb")
    {
        (void)mode;
        return File(fopen(path, "rb"));
    }
};

extern SDClass SD;
