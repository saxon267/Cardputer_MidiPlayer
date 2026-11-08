#pragma once
#include <stdint.h>
#include "config.h"

class Piano {
public:
    static constexpr int V = MAX_VOICES;
    static constexpr int P = MAX_PARTIALS;

    void init();
    void reset();
    void noteOn(uint8_t note, uint8_t vel, uint8_t ch = 0);
    void noteOff(uint8_t note, uint8_t ch = 0);
    void setSustain(bool on);
    void allOff();
    void render(int16_t* out, int frames, float master);
    void fillMask(volatile uint32_t* m) const;
    uint8_t activeCount() const;

private:
    struct Voice {
        bool     active   = false;
        bool     released = false;
        bool     sustained= false;
        uint8_t  note     = 0;
        uint8_t  vel      = 0;
        uint8_t  ch       = 0;
        uint32_t age      = 0;
        float    phase[P] = {0};
        float    inc[P]   = {0};
        float    amp[P]   = {0};
        float    decay[P] = {0};
        float    env      = 0;
        float    attack   = 0;
        float    rel      = 0;
        float    gain     = 0;
        float    noiseEnv = 0;
        uint32_t lfsr     = 1;
    };

    Voice    _v[V];
    float    _sine[1025];
    bool     _sustain = false;
    uint32_t _age = 0;
    float    _lim   = 1.0f;
    float    _dcX   = 0.0f;
    float    _dcY   = 0.0f;
    float    _hpX   = 0.0f;
    float    _hpY   = 0.0f;
};
