#include <cstdio>
#include <cstdint>
#include <cstring>
#include <SD.h>
#include "midi_recorder.h"

int64_t g_test_us = 1000000;
SDClass SD;

int main()
{
    rec.toggleChannel(1);   // CH2
    rec.toggleChannel(9);   // CH10

    rec.startRecording(1200);  // 120 BPM
    g_test_us += 0;
    rec.add(0x90, 60, 100);       // CH1 note on
    g_test_us += 250000;
    rec.add(0x91, 64, 90);        // CH2 note on
    g_test_us += 250000;
    rec.add(0x80, 60, 0);         // CH1 note off
    g_test_us += 100000;
    rec.add(0xB0, 64, 127);       // CH1 sustain on
    g_test_us += 100000;
    rec.add(0xB0, 64, 0);         // CH1 sustain off
    g_test_us += 100000;
    rec.add(0x81, 64, 0);         // CH2 note off
    g_test_us += 100000;
    rec.add(0x99, 36, 100);       // CH10 drum
    g_test_us += 100000;
    rec.add(0x89, 36, 0);
    g_test_us += 100000;
    rec.add(0x92, 67, 100);       // CH3 not selected -> ignored
    rec.add(0x82, 67, 0);
    rec.stopRecording();

    char path[64] = {0};
    if (!rec.save(path, sizeof(path))) {
        std::printf("SAVE FAILED\n");
        return 1;
    }
    std::printf("saved=%s events=%d\n", path, rec.count());

    char real[96];
    snprintf(real, sizeof(real), "test_out%s", path);
    FILE* f = fopen(real, "rb");
    if (!f) { std::printf("READ FAILED %s\n", real); return 1; }
    uint8_t buf[4096];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    std::printf("size=%u\n", (unsigned)n);
    if (n < 22 || memcmp(buf, "MThd", 4) != 0 ||
        memcmp(buf + 14, "MTrk", 4) != 0) {
        std::printf("RESULT FAIL header\n");
        return 1;
    }
    uint32_t tlen = (buf[18] << 24) | (buf[19] << 16) | (buf[20] << 8) | buf[21];
    std::printf("trackLen=%u\n", tlen);
    if (tlen + 22 != n) {
        std::printf("RESULT FAIL length\n");
        return 1;
    }
    if (memcmp(buf + 23, "\xFF\x51\x03", 3) != 0) {
        std::printf("RESULT FAIL tempo\n");
        return 1;
    }
    if (buf[n - 4] != 0 || buf[n - 3] != 0xFF || buf[n - 2] != 0x2F || buf[n - 1] != 0) {
        std::printf("RESULT FAIL end\n");
        return 1;
    }

    // Walk events after the delta 0 + 6-byte tempo meta. Assert only selected
    // channels (1, 2, 10) are present and that CH3 was filtered out.
    const uint8_t* p = buf + 22 + 7;
    const uint8_t* end = buf + 22 + tlen - 4;
    int ev = 0, ch1 = 0, ch2 = 0, ch10 = 0, ch3 = 0, cc64 = 0;
    while (p < end) {
        uint32_t delta = 0;
        while (p < end && (*p & 0x80)) delta = (delta << 7) | (*p++ & 0x7F);
        if (p >= end) break;
        delta = (delta << 7) | *p++;
        (void)delta;
        uint8_t st = *p++;
        if (st == 0xFF) {
            uint8_t type = *p++;
            uint8_t len = *p++;
            p += len;
            continue;
        }
        if ((st & 0x80) == 0) {
            std::printf("RESULT FAIL running status\n");
            return 1;
        }
        uint8_t ch = st & 0x0F;
        p += 2;
        ++ev;
        if (ch == 0) ++ch1;
        else if (ch == 1) ++ch2;
        else if (ch == 9) ++ch10;
        else if (ch == 2) ++ch3;
        if ((st & 0xF0) == 0xB0) ++cc64;
    }
    std::printf("events=%d ch1=%d ch2=%d ch10=%d ch3=%d cc=%d\n",
                ev, ch1, ch2, ch10, ch3, cc64);
    if (ev != 8 || ch1 < 4 || ch2 < 2 || ch10 < 2 || ch3 != 0 || cc64 != 2) {
        std::printf("RESULT FAIL channels\n");
        return 1;
    }
    std::printf("RESULT OK\n");
    return 0;
}
