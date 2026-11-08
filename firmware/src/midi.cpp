#include "midi.h"

#include <Arduino.h>
#include <SD.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

struct Tempo {
    uint32_t tick;
    uint32_t us;
};

struct Reader {
    File*    f;
    uint8_t  buf[512];
    uint16_t idx = 0;
    uint16_t len = 0;
    uint32_t pos = 0;

    bool fill()
    {
        int n = f->read(buf, sizeof(buf));
        if (n <= 0) return false;
        idx = 0;
        len = (uint16_t)n;
        return true;
    }

    int get()
    {
        if (idx >= len && !fill()) return -1;
        ++pos;
        return buf[idx++];
    }

    bool skip(uint32_t n)
    {
        while (n) {
            if (idx >= len && !fill()) return false;
            uint32_t avail = (uint32_t)(len - idx);
            uint32_t k = n < avail ? n : avail;
            idx = (uint16_t)(idx + k);
            pos += k;
            n -= k;
        }
        return true;
    }

    bool read4(char* out)
    {
        for (int i = 0; i < 4; ++i) {
            int c = get();
            if (c < 0) return false;
            out[i] = (char)c;
        }
        return true;
    }

    uint32_t read32()
    {
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i) v = (v << 8) | (uint32_t)(get() & 0xFF);
        return v;
    }

    uint16_t read16()
    {
        uint16_t v = 0;
        for (int i = 0; i < 2; ++i) v = (uint16_t)((v << 8) | (uint16_t)(get() & 0xFF));
        return v;
    }

    uint32_t vlq()
    {
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i) {
            int c = get();
            if (c < 0) return 0xFFFFFFFFu;
            v = (v << 7) | (uint32_t)(c & 0x7F);
            if (!(c & 0x80)) return v;
        }
        return 0xFFFFFFFFu;
    }
};

struct Ctx {
    MidiEvent* ev       = nullptr;
    size_t     cap      = 0;
    size_t     n        = 0;
    bool       fill     = false;
    bool       countOnly= false;
    bool       overflow = false;
    bool       tempoOver= false;
    uint32_t   maxTick  = 0;
    uint32_t   notes    = 0;

    Tempo    tempos[MAX_TEMPOS];
    int      tn = 0;
    bool     gotKey = false;
    int8_t   keySf  = 0;
    uint8_t  keyMi  = 0;
    bool     gotTitle = false;
    char     title[28] = {0};

    uint16_t tracks   = 0;
    uint16_t division = 0;
    uint8_t  tsNum    = 4;
    uint8_t  tsDen    = 4;
    bool     gotTs    = false;
};

void setErr(char* err, size_t len, const char* msg)
{
    if (err && len) snprintf(err, len, "%s", msg);
}

void addEvent(Ctx& c, uint32_t tick, uint8_t type, uint8_t note = 0, uint8_t vel = 0,
              uint8_t ch = 0)
{
    if (tick > c.maxTick) c.maxTick = tick;
    if (type == EV_ON) ++c.notes;

    if (c.fill) {
        if (c.n < c.cap) {
            c.ev[c.n] = MidiEvent{tick, type, note, vel, ch};
        } else {
            c.overflow = true;
        }
    } else if (!c.countOnly && c.n >= (size_t)MAX_EVENTS) {
        c.overflow = true;
    }
    ++c.n;
}

void addTempo(Ctx& c, uint32_t tick, uint32_t us)
{
    if (us == 0) return;
    if (c.tn < MAX_TEMPOS) {
        c.tempos[c.tn].tick = tick;
        c.tempos[c.tn].us   = us;
        ++c.tn;
    } else {
        c.tempoOver = true;
    }
}

int tempoCmp(const void* a, const void* b)
{
    const Tempo* x = (const Tempo*)a;
    const Tempo* y = (const Tempo*)b;
    if (x->tick < y->tick) return -1;
    if (x->tick > y->tick) return 1;
    return 0;
}

void sortTempos(Ctx& c)
{
    qsort(c.tempos, c.tn, sizeof(Tempo), tempoCmp);
    // keep the last change at a given tick
    int w = 0;
    for (int i = 0; i < c.tn; ++i) {
        if (w > 0 && c.tempos[w - 1].tick == c.tempos[i].tick) c.tempos[w - 1] = c.tempos[i];
        else c.tempos[w++] = c.tempos[i];
    }
    c.tn = w;
}

uint32_t tickToUs(uint32_t tick, const Tempo* t, int n, uint16_t division)
{
    if (!division) division = 480;
    uint64_t us   = 0;
    uint32_t last = 0;
    uint32_t cur  = 500000;  // 120 BPM
    for (int i = 0; i < n; ++i) {
        if (t[i].tick > tick) break;
        us += (uint64_t)(t[i].tick - last) * cur / division;
        last = t[i].tick;
        cur  = t[i].us ? t[i].us : cur;
    }
    us += (uint64_t)(tick - last) * cur / division;
    if (us > 0xFFFFFFFFull) us = 0xFFFFFFFFull;
    return (uint32_t)us;
}

void buildKey(Ctx& c, char* out, size_t n)
{
    static const char* majSharp[] = {"C", "G", "D", "A", "E", "B", "F#", "C#"};
    static const char* majFlat[]  = {"C", "F", "BB", "EB", "AB", "DB", "GB", "CB"};
    static const char* minSharp[] = {"A", "E", "B", "F#", "C#", "G#", "D#", "A#"};
    static const char* minFlat[]  = {"A", "D", "G", "C", "F", "BB", "EB", "AB"};

    if (!c.gotKey) {
        snprintf(out, n, "KEY --");
        return;
    }
    int sf = (int)c.keySf;
    if (sf > 7) sf = 7;
    if (sf < -7) sf = -7;
    const char* name;
    if (c.keyMi) name = sf >= 0 ? minSharp[sf] : minFlat[-sf];
    else         name = sf >= 0 ? majSharp[sf] : majFlat[-sf];
    snprintf(out, n, "%s %s", name, c.keyMi ? "MIN" : "MAJ");
}

bool parseFile(const char* path, Ctx& c, char* err, size_t errLen)
{
    File f = SD.open(path, FILE_READ);
    if (!f) {
        setErr(err, errLen, "OPEN FAILED");
        return false;
    }
    uint32_t fsize = (uint32_t)f.size();

    Reader r;
    r.f = &f;

    char id[4];
    if (!r.read4(id) || memcmp(id, "MThd", 4) != 0) {
        setErr(err, errLen, "NOT A MIDI FILE");
        f.close();
        return false;
    }
    uint32_t hlen = r.read32();
    if (hlen < 6) {
        setErr(err, errLen, "BAD HEADER");
        f.close();
        return false;
    }
    uint16_t format = r.read16();
    uint16_t ntrks  = r.read16();
    uint16_t div    = r.read16();
    if (hlen > 6) r.skip(hlen - 6);

    if (format > 1) {
        setErr(err, errLen, "FORMAT 2 UNSUPPORTED");
        f.close();
        return false;
    }
    if (ntrks == 0 || ntrks > MAX_TRACKS) {
        setErr(err, errLen, "BAD TRACK COUNT");
        f.close();
        return false;
    }
    if (div == 0 || (div & 0x8000)) {
        setErr(err, errLen, "SMPTE UNSUPPORTED");
        f.close();
        return false;
    }

    c.tracks   = ntrks;
    c.division = div;

    for (uint16_t tr = 0; tr < ntrks; ++tr) {
        char tid[4];
        if (!r.read4(tid) || memcmp(tid, "MTrk", 4) != 0) {
            setErr(err, errLen, "BAD TRACK");
            f.close();
            return false;
        }
        uint32_t len = r.read32();
        if (len > fsize || r.pos > fsize - len) {
            setErr(err, errLen, "BAD TRACK LEN");
            f.close();
            return false;
        }
        uint32_t trackEnd = r.pos + len;
        uint32_t tick     = 0;
        uint8_t  running  = 0;

        while (r.pos < trackEnd) {
            uint32_t delta = r.vlq();
            if (delta == 0xFFFFFFFFu) {
                setErr(err, errLen, "BAD DELTA");
                f.close();
                return false;
            }
            tick += delta;

            int st = r.get();
            if (st < 0) {
                setErr(err, errLen, "TRUNCATED");
                f.close();
                return false;
            }

            uint8_t status = 0;
            uint8_t d1     = 0;
            if (st < 0x80) {
                if (!running) {
                    setErr(err, errLen, "BAD RUNNING STATUS");
                    f.close();
                    return false;
                }
                status = running;
                d1     = (uint8_t)st;
            } else {
                status = (uint8_t)st;
                if (status < 0xF0) running = status;
            }

            if (status == 0xFF) {
                // In SMF, meta events do not cancel running status.
                int type = r.get();
                if (type < 0) {
                    setErr(err, errLen, "BAD META");
                    f.close();
                    return false;
                }
                uint32_t l = r.vlq();
                if (r.pos > trackEnd || l == 0xFFFFFFFFu || l > trackEnd - r.pos) {
                    setErr(err, errLen, "BAD META LEN");
                    f.close();
                    return false;
                }

                if (type == 0x51 && l == 3) {
                    uint32_t us = ((uint32_t)r.get() << 16) | ((uint32_t)r.get() << 8) | (uint32_t)r.get();
                    addTempo(c, tick, us);
                } else if (type == 0x59 && l >= 2) {
                    c.keySf  = (int8_t)r.get();
                    c.keyMi  = (uint8_t)r.get();
                    c.gotKey = true;
                    if (l > 2 && !r.skip(l - 2)) {
                        setErr(err, errLen, "TRUNCATED");
                        f.close();
                        return false;
                    }
                } else if (type == 0x58 && l >= 2) {
                    c.tsNum = (uint8_t)r.get();
                    uint8_t dd = (uint8_t)r.get();
                    c.tsDen = (uint8_t)(1u << (dd > 7 ? 7 : dd));
                    c.gotTs = true;
                    if (l > 2 && !r.skip(l - 2)) {
                        setErr(err, errLen, "TRUNCATED");
                        f.close();
                        return false;
                    }
                } else if (type == 0x03 && !c.gotTitle) {
                    uint32_t k = l < sizeof(c.title) - 1 ? l : sizeof(c.title) - 1;
                    for (uint32_t i = 0; i < k; ++i) {
                        int ch = r.get();
                        c.title[i] = (ch >= 32 && ch < 127) ? (char)ch : ' ';
                    }
                    c.title[k] = 0;
                    c.gotTitle = true;
                    if (l > k && !r.skip(l - k)) {
                        setErr(err, errLen, "TRUNCATED");
                        f.close();
                        return false;
                    }
                } else {
                    if (!r.skip(l)) {
                        setErr(err, errLen, "TRUNCATED");
                        f.close();
                        return false;
                    }
                }

                if (type == 0x2F) {  // end of track
                    if (r.pos < trackEnd && !r.skip(trackEnd - r.pos)) {
                        setErr(err, errLen, "TRUNCATED");
                        f.close();
                        return false;
                    }
                    break;
                }
                continue;
            }

            if (status == 0xF0 || status == 0xF7) {
                running = 0;
                uint32_t l = r.vlq();
                if (r.pos > trackEnd || l == 0xFFFFFFFFu || l > trackEnd - r.pos) {
                    setErr(err, errLen, "BAD SYSEX");
                    f.close();
                    return false;
                }
                if (!r.skip(l)) {
                    setErr(err, errLen, "TRUNCATED");
                    f.close();
                    return false;
                }
                continue;
            }
            if (status >= 0xF1) {                setErr(err, errLen, "BAD STATUS");
                f.close();
                return false;
            }

            uint8_t hi = status & 0xF0;
            uint8_t ch = status & 0x0F;
            if (st >= 0x80) {
                int v = r.get();
                if (v < 0) {
                    setErr(err, errLen, "TRUNCATED");
                    f.close();
                    return false;
                }
                d1 = (uint8_t)v;
            }

            uint8_t d2 = 0;
            switch (hi) {
                case 0x80:  // note off
                case 0x90:  // note on
                case 0xA0:  // poly pressure
                case 0xB0:  // control change
                case 0xE0:  // pitch bend
                {
                    int v = r.get();
                    if (v < 0) {
                        setErr(err, errLen, "TRUNCATED");
                        f.close();
                        return false;
                    }
                    d2 = (uint8_t)v;
                    break;
                }
                case 0xC0:  // program change
                case 0xD0:  // channel pressure
                default:
                    break;
            }

            if (ch == 9) continue;  // GM drums: no piano

            if (hi == 0x90) {
                if (d2 == 0) addEvent(c, tick, EV_OFF, d1, 0, ch);
                else         addEvent(c, tick, EV_ON, d1, d2, ch);
            } else if (hi == 0x80) {
                addEvent(c, tick, EV_OFF, d1, 0, ch);
            } else if (hi == 0xB0) {
                if (d1 == 64) addEvent(c, tick, d2 >= 64 ? EV_SUS_ON : EV_SUS_OFF, 0, 0, ch);
                else if (d1 == 120 || d1 == 123) addEvent(c, tick, EV_ALL_OFF, 0, 0, ch);
            }
        }

        if (r.pos < trackEnd) {
            if (!r.skip(trackEnd - r.pos)) {
                setErr(err, errLen, "TRUNCATED");
                f.close();
                return false;
            }
        }
        if (r.pos > trackEnd) {
            setErr(err, errLen, "TRACK OVERRUN");
            f.close();
            return false;
        }
        if (tick > c.maxTick) c.maxTick = tick;
    }

    f.close();
    sortTempos(c);
    return true;
}

int evCmp(const void* a, const void* b)
{
    const MidiEvent* x = (const MidiEvent*)a;
    const MidiEvent* y = (const MidiEvent*)b;
    if (x->t != y->t) return x->t < y->t ? -1 : 1;
    if (x->type != y->type) return x->type < y->type ? -1 : 1;
    if (x->note != y->note) return x->note < y->note ? -1 : 1;
    return 0;
}

}  // namespace

void MidiFile::clear()
{
    if (_ev) free(_ev);
    _ev       = nullptr;
    _n        = 0;
    _duration = 0;
}

bool MidiFile::probe(const char* path, MidiInfo& info)
{
    Ctx c;
    c.countOnly = true;
    c.fill      = false;

    char err[48];
    if (!parseFile(path, c, err, sizeof(err))) return false;

    info = MidiInfo{};
    info.tracks     = c.tracks;
    info.division   = c.division;
    info.maxTick    = c.maxTick;
    info.tsNum      = c.tsNum;
    info.tsDen      = c.tsDen;
    info.noteCount  = c.notes;
    info.durationUs = tickToUs(c.maxTick, c.tempos, c.tn, c.division);
    info.bpmX10     = c.tn ? (uint16_t)(600000000ull / c.tempos[0].us) : 1200;
    buildKey(c, info.key, sizeof(info.key));
    strncpy(info.title, c.title, sizeof(info.title) - 1);
    info.valid = c.notes > 0;
    return info.valid;
}

bool MidiFile::load(const char* path, MidiInfo& info, char* err, size_t errLen)
{
    clear();

    Ctx c1;
    c1.fill = false;
    if (!parseFile(path, c1, err, errLen)) return false;
    if (c1.overflow) {
        setErr(err, errLen, "TOO COMPLEX");
        return false;
    }
    if (c1.tempoOver) {
        setErr(err, errLen, "TOO MANY TEMPOS");
        return false;
    }
    if (c1.notes == 0) {
        setErr(err, errLen, "NO NOTES");
        return false;
    }
    if (c1.n == 0) {
        setErr(err, errLen, "EMPTY");
        return false;
    }

    _ev = (MidiEvent*)malloc(c1.n * sizeof(MidiEvent));
    if (!_ev) {
        setErr(err, errLen, "OUT OF MEMORY");
        return false;
    }

    Ctx c2;
    c2.ev   = _ev;
    c2.cap  = c1.n;
    c2.fill = true;
    if (!parseFile(path, c2, err, errLen) || c2.overflow || c2.n != c1.n) {
        clear();
        if (!err[0]) setErr(err, errLen, "PARSE ERROR");
        return false;
    }

    _n = c2.n;
    qsort(_ev, _n, sizeof(MidiEvent), evCmp);

    for (size_t i = 0; i < _n; ++i)
        _ev[i].t = tickToUs(_ev[i].t, c2.tempos, c2.tn, c2.division);

    _duration = tickToUs(c2.maxTick, c2.tempos, c2.tn, c2.division);

    info = MidiInfo{};
    info.tracks     = c2.tracks;
    info.division   = c2.division;
    info.maxTick    = c2.maxTick;
    info.tsNum      = c2.tsNum;
    info.tsDen      = c2.tsDen;
    info.noteCount  = c1.notes;
    info.durationUs = _duration;
    info.bpmX10     = c2.tn ? (uint16_t)(600000000ull / c2.tempos[0].us) : 1200;
    buildKey(c2, info.key, sizeof(info.key));
    strncpy(info.title, c2.title, sizeof(info.title) - 1);
    info.valid = true;

    return true;
}
