#include <cstdio>
#include <cstdint>
#include <cmath>
#include "synth.h"

int main()
{
    Piano p;
    p.init();
    p.noteOn(60, 110);

    int16_t buf[256];
    for (int i = 0; i < 40; ++i) p.render(buf, 256, 0.55f);

    p.noteOff(60, 0);
    int blocks = 0;
    while (p.activeCount() != 0 && blocks < 120) {
        p.render(buf, 256, 0.55f);
        ++blocks;
    }

    float ms = blocks * 256.0f * 1000.0f / 44100.0f;
    std::printf("release blocks=%d ms=%.0f active=%u\n", blocks, ms, p.activeCount());
    if (p.activeCount() != 0 || ms > 600.0f) {
        std::printf("RESULT FAIL\n");
        return 1;
    }
    std::printf("RESULT OK\n");
    return 0;
}
