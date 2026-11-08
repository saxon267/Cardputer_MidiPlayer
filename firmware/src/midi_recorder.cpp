#include "midi_recorder.h"

#if USB_MIDI_HOST
#include <Arduino.h>
#include <SD.h>
#include <esp_timer.h>
#include <stdio.h>
#include <string.h>

MidiRecorder rec;

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

void MidiRecorder::clearLocked()
{
    _n = 0;
    _full = false;
    _fullRecording = false;
    _durationUs = 0;
}

void MidiRecorder::beginPreview()
{
    portENTER_CRITICAL(&s_mux);
    clearLocked();
    _active = true;
    _recording = false;
    _armed = false;
    _startUs = (uint32_t)esp_timer_get_time();
    portEXIT_CRITICAL(&s_mux);
}

bool MidiRecorder::startRecording(uint16_t bpmX10)
{
    if (bpmX10 < 200) bpmX10 = 200;
    if (bpmX10 > 4000) bpmX10 = 4000;
    portENTER_CRITICAL(&s_mux);
    clearLocked();
    _bpmX10 = bpmX10;
    _active = true;
    _recording = true;
    _armed = true;
    _startUs = 0;
    portEXIT_CRITICAL(&s_mux);
    return true;
}

void MidiRecorder::stopRecording()
{
    portENTER_CRITICAL(&s_mux);
    if (_recording) {
        uint32_t now = (uint32_t)esp_timer_get_time();
        _durationUs = _armed ? 0 : (now - _startUs);
    }
    _recording = false;
    _active = false;
    _armed = false;
    portEXIT_CRITICAL(&s_mux);
}

uint32_t MidiRecorder::durationUs() const
{
    return _durationUs;
}

uint32_t MidiRecorder::elapsedUs() const
{
    if (!_active) return _durationUs;
    if (_armed) return 0;
    return (uint32_t)esp_timer_get_time() - _startUs;
}

void MidiRecorder::add(uint16_t status, uint8_t d1, uint8_t d2)
{
    const uint8_t hi = status & 0xF0;
    const uint8_t ch = status & 0x0F;
    if (!(_mask & (1u << ch))) return;

    uint8_t type = 0;
    uint8_t note = 0;
    uint8_t vel  = 0;
    if (hi == 0x90) {
        if (d2 == 0) { type = REC_OFF; note = d1; }
        else         { type = REC_ON;  note = d1; vel = d2; }
    } else if (hi == 0x80) {
        type = REC_OFF;
        note = d1;
    } else if (hi == 0xB0 && d1 == 64) {
        type = REC_CC64;
        vel  = d2;
    } else {
        return;
    }

    portENTER_CRITICAL(&s_mux);
    if (!_active) {
        portEXIT_CRITICAL(&s_mux);
        return;
    }
    uint32_t now = (uint32_t)esp_timer_get_time();
    if (_armed) {
        _startUs = now;
        _armed = false;
    }
    if (_n >= MAX_EVENTS) {
        _full = true;
        _fullRecording = _recording;
        _durationUs = now - _startUs;
        _recording = false;
        _active = false;
        portEXIT_CRITICAL(&s_mux);
        return;
    }
    RecEvent& e = _ev[_n];
    e.t = now - _startUs;
    e.type = type;
    e.note = note;
    e.vel = vel;
    e.ch = ch;
    ++_n;
    portEXIT_CRITICAL(&s_mux);
}

void MidiRecorder::toggleChannel(uint8_t ch)
{
    if (_recording) return;
    const uint32_t bit = 1u << (ch & 15);
    uint32_t next = _mask ^ bit;
    if (next == 0) return;  // keep at least one channel selected
    _mask = next;
}

int MidiRecorder::vlqLen(uint32_t v)
{
    if (v < 0x80) return 1;
    if (v < 0x4000) return 2;
    if (v < 0x200000) return 3;
    return 4;
}

void MidiRecorder::writeVlq(uint8_t*& p, uint32_t v)
{
    uint8_t tmp[4];
    int n = 0;
    tmp[n++] = v & 0x7F;
    while ((v >>= 7) != 0) tmp[n++] = (v & 0x7F) | 0x80;
    while (n > 0) *p++ = tmp[--n];
}

bool MidiRecorder::save(char* outPath, size_t outLen)
{
    if (_recording || _n == 0) return false;

    SD.mkdir("/midi");
    SD.mkdir("/midi/MyMIDI");

    char prefix[24] = {0};
    auto addPrefix = [&](const char* p) {
        if (prefix[0]) strncat(prefix, "_", sizeof(prefix) - strlen(prefix) - 1);
        strncat(prefix, p, sizeof(prefix) - strlen(prefix) - 1);
    };
    if (_mask & (1u << 0)) addPrefix("LEAD");
    if (_mask & (1u << 1)) addPrefix("PAD");
    if (_mask & (1u << 2)) addPrefix("BASS");
    if (_mask & (1u << 9)) addPrefix("DRUM");
    if (!prefix[0]) strcpy(prefix, "MIDI");

    char path[64] = {0};
    int take = 0;
    for (int i = 1; i <= 9999; ++i) {
        snprintf(path, sizeof(path), "/midi/MyMIDI/%s_%04d.MID", prefix, i);
        if (!SD.exists(path)) { take = i; break; }
    }
    if (take == 0) return false;

    uint32_t tempoUs = 600000000u / _bpmX10;
    if (tempoUs > 0xFFFFFFu) tempoUs = 0xFFFFFFu;

    uint32_t trackLen = 7 + 4;  // delta 0 + tempo meta + end of track
    uint32_t prevTick = 0;
    for (int i = 0; i < _n; ++i) {
        uint64_t tick = (uint64_t)_ev[i].t * _bpmX10 * DIVISION / 600000000ull;
        uint32_t delta = (uint32_t)tick - prevTick;
        trackLen += vlqLen(delta) + 3;
        prevTick = (uint32_t)tick;
    }

    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;

    uint8_t hdr[22] = {
        'M','T','h','d', 0,0,0,6, 0,0, 0,1, (uint8_t)(DIVISION >> 8), (uint8_t)DIVISION,
        'M','T','r','k',
        (uint8_t)(trackLen >> 24), (uint8_t)(trackLen >> 16),
        (uint8_t)(trackLen >> 8),  (uint8_t)trackLen
    };
    f.write(hdr, sizeof(hdr));

    uint8_t tempo[7] = {0x00, 0xFF, 0x51, 0x03,
                        (uint8_t)(tempoUs >> 16), (uint8_t)(tempoUs >> 8), (uint8_t)tempoUs};
    f.write(tempo, sizeof(tempo));

    prevTick = 0;
    for (int i = 0; i < _n; ++i) {
        uint64_t tick = (uint64_t)_ev[i].t * _bpmX10 * DIVISION / 600000000ull;
        uint32_t delta = (uint32_t)tick - prevTick;
        prevTick = (uint32_t)tick;

        uint8_t buf[8];
        uint8_t* p = buf;
        writeVlq(p, delta);
        const RecEvent& e = _ev[i];
        if (e.type == REC_ON) {
            *p++ = (uint8_t)(0x90 | e.ch);
            *p++ = e.note;
            *p++ = e.vel;
        } else if (e.type == REC_OFF) {
            *p++ = (uint8_t)(0x80 | e.ch);
            *p++ = e.note;
            *p++ = 0;
        } else if (e.type == REC_CC64) {
            *p++ = (uint8_t)(0xB0 | e.ch);
            *p++ = 64;
            *p++ = e.vel;
        }
        f.write(buf, (size_t)(p - buf));
    }

    uint8_t end[4] = {0x00, 0xFF, 0x2F, 0x00};
    f.write(end, sizeof(end));
    f.close();

    if (outPath && outLen) {
        strncpy(outPath, path, outLen - 1);
        outPath[outLen - 1] = 0;
    }
    _full = false;
    _fullRecording = false;
    return true;
}
#endif
