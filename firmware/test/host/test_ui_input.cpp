#include <cstdio>
#include <cstring>
#include <SD.h>

#define private public
#include "app.h"
#include "audio.h"
#include "library.h"
#include "ui.h"
#undef private

#include "midi_recorder.h"

int64_t g_test_us = 1000000;
SDClass SD;
App app;
Library library;
AudioEngine audio;

unsigned long millis() { return 0; }

int main()
{
    app.screen = Screen::MIDI_IN;
    app.usbConnected = true;
    app.usbHostOk = true;
    app.bpmX10 = 1200;
    app.sdOk = true;
    app.battery = 80;
    app.info.bpmX10 = 1200;

    rec.toggleChannel(1);
    rec.toggleChannel(9);
    rec.beginPreview();

    rec.add(0x90, 60, 100);
    g_test_us += 300000;
    rec.add(0x91, 64, 92);
    g_test_us += 300000;
    rec.add(0x99, 36, 100);
    g_test_us += 300000;
    rec.add(0x80, 60, 0);
    g_test_us += 400000;
    rec.add(0x81, 64, 0);
    g_test_us += 300000;
    rec.add(0x89, 36, 0);
    g_test_us += 500000;

    ui.begin();
    g_ui_out = "ui_input.bmp";
    for (int i = 0; i < 10; ++i) ui.render();
    std::printf("events=%d duration=%.1f base=%d\n", rec.count(), rec.elapsedUs() / 1e6, ui._rollBase);
    for (int i = 0; i < rec.count(); ++i) {
        const RecEvent& e = rec.events()[i];
        std::printf("  %d type=%d note=%d ch=%d t=%.1f on=%d\n", i, e.type, e.note, e.ch,
                    e.t / 1e6, rec.channelOn(e.ch) ? 1 : 0);
    }
    return 0;
}
