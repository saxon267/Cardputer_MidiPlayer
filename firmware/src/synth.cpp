#include "synth.h"

#include <math.h>
#include <string.h>

void Piano::init()
{
    for (int i = 0; i <= 1024; ++i)
        _sine[i] = sinf((float)i * 6.2831853f / 1024.0f);
    reset();
}

void Piano::reset()
{
    memset(_v, 0, sizeof(_v));
    _sustain = false;
    _lim     = 1.0f;
    _dcX     = 0.0f;
    _dcY     = 0.0f;
    _hpX     = 0.0f;
    _hpY     = 0.0f;
}

void Piano::noteOn(uint8_t note, uint8_t vel, uint8_t ch)
{
    if (note > 127 || vel == 0) return;

    int idx = -1;
    for (int i = 0; i < V; ++i) {
        if (!_v[i].active) { idx = i; break; }
    }
    if (idx < 0) {
        float lowest = 2.0f;
        for (int i = 0; i < V; ++i) {
            if (_v[i].env < lowest) { lowest = _v[i].env; idx = i; }
        }
    }

    Voice& v = _v[idx];
    memset(&v, 0, sizeof(v));
    v.active = true;
    v.note   = note;
    v.vel    = vel;
    v.ch     = ch;
    v.age    = ++_age;

    float f  = 440.0f * powf(2.0f, (note - 69) / 12.0f);
    float vv = vel / 127.0f;
    float tauScale = 261.63f / f;
    if (tauScale < 0.40f) tauScale = 0.40f;
    if (tauScale > 1.80f) tauScale = 1.80f;

    const float ratio[3] = {1.0f, 2.001f, 3.006f};
    const float amps[3]  = {1.0f, 0.42f, 0.18f};
    const float taus[3]  = {1.8f, 1.2f, 0.8f};

    for (int k = 0; k < P; ++k) {
        v.phase[k] = 0;
        v.inc[k]   = f * ratio[k] * 1024.0f / (float)AUDIO_RATE;
        v.amp[k]   = amps[k] * (k == 0 ? 1.0f : (0.45f + 0.55f * vv));
        v.decay[k] = expf(-1.0f / (taus[k] * tauScale * (float)AUDIO_RATE));
    }

    v.attack = 1.0f / (0.004f * (float)AUDIO_RATE);
    v.rel    = expf(-1.0f / (0.05f * (float)AUDIO_RATE));
    v.gain   = 0.16f * powf(vv, 1.3f);
    v.noiseEnv = 0.35f;
    v.lfsr   = 0x9E3779B9u ^ ((uint32_t)note * 2654435761u);
}

void Piano::noteOff(uint8_t note, uint8_t ch)
{
    for (int i = 0; i < V; ++i) {
        Voice& v = _v[i];
        if (!v.active || v.released || v.note != note || v.ch != ch) continue;
        if (_sustain) v.sustained = true;
        else          v.released  = true;
    }
}

void Piano::setSustain(bool on)
{
    _sustain = on;
    if (!on) {
        for (int i = 0; i < V; ++i) {
            Voice& v = _v[i];
            if (v.active && v.sustained) {
                v.sustained = false;
                v.released  = true;
            }
        }
    }
}

void Piano::allOff()
{
    for (int i = 0; i < V; ++i) {
        Voice& v = _v[i];
        if (v.active) {
            v.sustained = false;
            v.released  = true;
        }
    }
}

void Piano::render(int16_t* out, int frames, float master)
{
    static float acc[256];
    if (frames > 256) frames = 256;
    memset(acc, 0, sizeof(float) * frames);

    for (int vi = 0; vi < V; ++vi) {
        Voice& v = _v[vi];
        if (!v.active) continue;

        for (int i = 0; i < frames; ++i) {
            if (!v.released && v.env < 1.0f) {
                v.env += v.attack;
                if (v.env > 1.0f) v.env = 1.0f;
            }
            if (v.released) {
                v.env *= v.rel;
            }
            if (v.env < 0.0007f) {
                v.active = false;
                break;
            }

            float s = 0;
            for (int k = 0; k < P; ++k) {
                float ph = v.phase[k] + v.inc[k];
                if (ph >= 1024.0f) ph -= 1024.0f;
                v.phase[k] = ph;
                int   ip = (int)ph;
                float fr = ph - (float)ip;
                float a  = _sine[ip] + (_sine[ip + 1] - _sine[ip]) * fr;
                s += v.amp[k] * a;
                v.amp[k] *= v.decay[k];
                if (v.amp[k] < 1e-6f) v.amp[k] = 0.0f;
            }

            if (v.noiseEnv > 0.002f) {
                v.lfsr = v.lfsr * 1664525u + 1013904223u;
                float n = (float)((int32_t)v.lfsr >> 9) * (1.0f / 4194304.0f);
                s += n * v.noiseEnv;
                v.noiseEnv *= 0.997f;
            }

            acc[i] += s * v.env * v.gain;
        }
    }

    for (int i = 0; i < frames; ++i) {
        float x = acc[i] * master;

        // Look-ahead-free limiter: fast attack, slow release. Keeps chords
        // below full scale instead of letting tanh hard-saturate them.
        float ax = fabsf(x);
        float target = ax > 0.85f ? 0.85f / ax : 1.0f;
        if (target < _lim) _lim += (target - _lim) * 0.35f;
        else               _lim += (1.0f - _lim) * 0.001f;
        x *= _lim;

        // Padé approximation of tanh: cheaper than libm tanh and plenty
        // accurate for the -3..3 range we feed it.
        if (x > 3.0f) x = 3.0f;
        if (x < -3.0f) x = -3.0f;
        float x2 = x * x;
        float y = x * (27.0f + x2) / (27.0f + 9.0f * x2) * 0.95f;

        float z = y - _dcX + 0.995f * _dcY;
        _dcX = y;
        _dcY = z;

        // 90 Hz high-pass: the 1 W speaker cannot reproduce the bottom end
        // and a boosted sub-bass only makes it clip.
        float h = z - _hpX + 0.987f * _hpY;
        _hpX = z;
        _hpY = h;
        z = h;

        int32_t s = (int32_t)(z * 32767.0f);
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        out[i] = (int16_t)s;
    }

    // With the pedal down a voice can be inaudible but still occupy a slot;
    // release it once all partials have decayed to zero.
    for (int vi = 0; vi < V; ++vi) {
        Voice& v = _v[vi];
        if (!v.active) continue;
        bool silent = true;
        for (int k = 0; k < P; ++k) {
            if (v.amp[k] != 0.0f) { silent = false; break; }
        }
        if (silent) v.active = false;
    }
}

void Piano::fillMask(volatile uint32_t* m) const
{
    m[0] = m[1] = m[2] = m[3] = 0;
    for (int i = 0; i < V; ++i) {
        const Voice& v = _v[i];
        if (!v.active || v.note > 127) continue;
        m[v.note >> 5] |= (1u << (v.note & 31));
    }
}

uint8_t Piano::activeCount() const
{
    uint8_t n = 0;
    for (int i = 0; i < V; ++i) if (_v[i].active) ++n;
    return n;
}
