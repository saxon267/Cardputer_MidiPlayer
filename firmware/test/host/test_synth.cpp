#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include "synth.h"

static void wavHeader(FILE* f, uint32_t frames, uint32_t rate)
{
    uint32_t data = frames * 2;
    uint32_t riff = 36 + data;
    fwrite("RIFF", 1, 4, f);
    fwrite(&riff, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f);
    uint32_t fmt = 16;
    uint16_t pcm = 1, ch = 1, bits = 16;
    uint32_t byteRate = rate * 2;
    uint16_t align = 2;
    fwrite(&fmt, 4, 1, f);
    fwrite(&pcm, 2, 1, f);
    fwrite(&ch, 2, 1, f);
    fwrite(&rate, 4, 1, f);
    fwrite(&byteRate, 4, 1, f);
    fwrite(&align, 2, 1, f);
    fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f);
    fwrite(&data, 4, 1, f);
}

int main(int argc, char** argv)
{
    const char* out = argc > 1 ? argv[1] : "demo.wav";
    Piano p;
    p.init();
    p.noteOn(60, 110);
    p.noteOn(64, 100);
    p.noteOn(67, 96);

    const int rate = AUDIO_RATE;
    const int total = rate * 3;  // 3 s
    int16_t buf[256];
    int16_t* all = new int16_t[total];
    int pos = 0;
    int peak = 0;
    double sum = 0;

    for (int n = 0; n < total; n += 128) {
        if (n == rate * 2) {  // release after 2 s
            p.noteOff(60); p.noteOff(64); p.noteOff(67);
        }
        p.render(buf, 128, 0.78f * 0.78f * 1.2f);
        for (int i = 0; i < 128 && pos < total; ++i) {
            all[pos++] = buf[i];
            int a = buf[i] < 0 ? -buf[i] : buf[i];
            if (a > peak) peak = a;
            sum += (double)buf[i] * buf[i];
        }
    }

    FILE* f = fopen(out, "wb");
    if (!f) { std::printf("cannot write %s\n", out); return 1; }
    wavHeader(f, total, rate);
    fwrite(all, 2, total, f);
    fclose(f);

    double rms = std::sqrt(sum / total);
    std::printf("wrote %s  frames=%d peak=%d rms=%.1f active=%u\n", out, total, peak, rms,
                (unsigned)p.activeCount());

    int bad = 0;
    if (peak < 2000) { std::printf("BAD: too quiet\n"); ++bad; }
    if (peak > 32767) { std::printf("BAD: overflow\n"); ++bad; }
    if (rms < 100) { std::printf("BAD: rms too low\n"); ++bad; }
    std::printf(bad ? "RESULT FAIL\n" : "RESULT OK\n");
    delete[] all;
    return bad ? 1 : 0;
}
