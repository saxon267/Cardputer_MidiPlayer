#pragma once
#include "config.h"
#include "library.h"
#include "midi.h"

enum class Screen : uint8_t { BOOT, LIBRARY, PLAYER, ROLL, MIDI_IN, NO_SD, NO_FILES };

struct App {
    Screen   screen     = Screen::BOOT;
    int      sel        = 0;
    int      scroll     = 0;
    int      playIndex  = -1;
    uint8_t  volume     = 78;
    uint16_t bpmX10     = 1200;
    bool     bpmFromFm1 = false;
    bool     loop       = false;
    uint8_t  battery    = 80;
    bool     sdOk       = false;
    bool     usbHostOk  = false;
    bool     usbDeviceSeen = false;
    bool     usbConnected = false;
    uint32_t bootStart  = 0;
    char     title[32]  = {0};
    char     toast[40]  = {0};
    uint32_t toastUntil = 0;
    char     err[40]    = {0};
    MidiInfo info;
    const MidiEvent* ev = nullptr;
    size_t   evCount    = 0;
};

extern App app;
