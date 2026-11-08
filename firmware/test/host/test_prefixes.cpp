#include <cstdio>
#include <cstring>
#include <SD.h>
#include "midi_recorder.h"

int64_t g_test_us = 1000000;
SDClass SD;

static bool take(const char* wantPrefix, uint8_t ch)
{
    rec.startRecording(1200);
    g_test_us += 1000;
    rec.add((uint16_t)(0x90 | ch), 60, 100);
    rec.add((uint16_t)(0x80 | ch), 60, 0);
    rec.stopRecording();
    char path[64] = {0};
    if (!rec.save(path, sizeof(path))) {
        std::printf("FAIL save %s\n", wantPrefix);
        return false;
    }
    char want[64];
    snprintf(want, sizeof(want), "%s_0001.MID", wantPrefix);
    if (!strstr(path, want)) {
        std::printf("FAIL prefix=%s got=%s\n", wantPrefix, path);
        return false;
    }
    std::printf("OK %s\n", path);
    return true;
}

int main()
{
    // Default: CH1 only.
    if (!take("LEAD", 0)) return 1;
    rec.toggleChannel(1);
    rec.toggleChannel(0);
    if (!take("PAD", 1)) return 1;
    rec.toggleChannel(2);
    rec.toggleChannel(1);
    if (!take("BASS", 2)) return 1;
    rec.toggleChannel(9);
    rec.toggleChannel(2);
    if (!take("DRUM", 9)) return 1;

    std::printf("RESULT OK\n");
    return 0;
}
