#pragma once
#include <stddef.h>
#include <stdint.h>
#include "config.h"

struct MidiEvent {
    uint32_t t;      // absolute time, microseconds
    uint8_t  type;
    uint8_t  note;
    uint8_t  vel;
    uint8_t  pad;    // MIDI channel (0..15), used for note pairing
};

enum : uint8_t {
    EV_OFF      = 0,
    EV_ON       = 1,
    EV_SUS_ON   = 2,
    EV_SUS_OFF  = 3,
    EV_ALL_OFF  = 4,
};

struct MidiInfo {
    uint32_t durationUs = 0;
    uint32_t maxTick    = 0;
    uint32_t noteCount  = 0;
    uint16_t tracks     = 0;
    uint16_t division   = 0;
    uint16_t bpmX10     = 1200;
    uint8_t  tsNum      = 4;
    uint8_t  tsDen      = 4;
    char     key[12]    = "KEY --";
    char     title[28]  = {0};
    bool     valid      = false;
};

class MidiFile {
public:
    bool probe(const char* path, MidiInfo& info);
    bool load(const char* path, MidiInfo& info, char* err, size_t errLen);
    void clear();

    const MidiEvent* events() const { return _ev; }
    size_t           count() const { return _n; }
    uint32_t         durationUs() const { return _duration; }

private:
    MidiEvent* _ev        = nullptr;
    size_t     _n         = 0;
    uint32_t   _duration  = 0;
};
