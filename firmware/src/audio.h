#pragma once
#include <stddef.h>
#include <stdint.h>
#include "config.h"
#include "midi.h"

class AudioEngine {
public:
    bool begin();
    void start();

    void setSong(const MidiEvent* events, size_t count, uint32_t durationUs);
    void clearSong();
    void play();
    void togglePause();
    void stop();
    void seek(int32_t deltaUs);
    void setLoop(bool on);
    void setVolume(uint8_t v);

    bool     playing() const { return _playing; }
    bool     paused() const { return _paused; }
    bool     ended() const { return _ended; }
    bool     loop() const { return _loop; }
    uint8_t  volume() const { return _volume; }
    uint32_t positionUs() const { return _posUs; }
    uint32_t durationUs() const { return _durationUs; }
    bool     noteActive(uint8_t note) const
    {
        return note < 128 && (_active[note >> 5] & (1u << (note & 31))) != 0;
    }
    uint8_t  cpuPct() const { return _cpu; }
    uint16_t underruns() const { return _underruns; }
    bool     codecOk() const { return _codecOk; }
    bool     sustain() const { return _sustainOn; }
    uint8_t  voices() const { return _voices; }

private:
    static void taskEntry(void* arg);
    void        taskLoop();
    void        apply(const MidiEvent& e);
    void        resetPlayback();
    void        handleSeek();

    const MidiEvent* volatile _events   = nullptr;
    volatile size_t           _count    = 0;
    volatile size_t           _idx      = 0;
    volatile uint32_t         _posUs    = 0;
    volatile uint64_t         _frames   = 0;
    volatile uint32_t         _durationUs = 0;
    volatile uint32_t         _seekTo   = 0;
    volatile bool             _seekReq  = false;
    volatile bool             _playing  = false;
    volatile bool             _paused   = false;
    volatile bool             _ended    = false;
    volatile bool             _loop     = false;
    volatile uint8_t          _volume   = 78;
    volatile uint8_t          _cpu      = 0;
    volatile uint16_t         _underruns = 0;
    volatile bool             _sustainOn = false;
    volatile uint8_t          _voices    = 0;
    volatile uint32_t         _active[4] = {0, 0, 0, 0};
    bool                      _codecOk  = false;
    void*                     _task     = nullptr;
};

extern AudioEngine audio;
