#pragma once
#include <stdint.h>
#include <stddef.h>

#if USB_MIDI_HOST

enum RecType : uint8_t {
    REC_ON   = 1,
    REC_OFF  = 2,
    REC_CC64 = 3,
};

struct RecEvent {
    uint32_t t;      // microseconds from record start
    uint8_t  type;
    uint8_t  note;
    uint8_t  vel;
    uint8_t  ch;     // 0..15
};

class MidiRecorder {
public:
    static constexpr int MAX_EVENTS = 6000;
    static constexpr uint16_t DIVISION = 480;

    void beginPreview();
    bool startRecording(uint16_t bpmX10);
    void stopRecording();
    bool save(char* outPath, size_t outLen);

    void add(uint16_t status, uint8_t d1, uint8_t d2);

    bool     recording() const { return _recording; }
    bool     armed() const { return _armed; }
    bool     active() const { return _active; }
    bool     full() const { return _full; }
    bool     fullTake() const { return _fullRecording; }
    int      count() const { return _n; }
    uint32_t durationUs() const;
    uint32_t elapsedUs() const;
    uint16_t bpmX10() const { return _bpmX10; }
    const RecEvent* events() const { return _ev; }

    uint32_t channelMask() const { return _mask; }
    bool     channelOn(uint8_t ch) const { return (_mask & (1u << (ch & 15))) != 0; }
    void     toggleChannel(uint8_t ch);

private:
    RecEvent _ev[MAX_EVENTS];
    volatile int _n = 0;
    volatile bool _active = false;
    volatile bool _recording = false;
    volatile bool _armed = false;
    volatile bool _full = false;
    volatile bool _fullRecording = false;
    uint32_t _startUs = 0;
    uint32_t _durationUs = 0;
    uint16_t _bpmX10 = 1200;
    uint32_t _mask = 1u << 0;   // CH1
    void clearLocked();
    static int vlqLen(uint32_t v);
    static void writeVlq(uint8_t*& p, uint32_t v);
};

extern MidiRecorder rec;

#endif
