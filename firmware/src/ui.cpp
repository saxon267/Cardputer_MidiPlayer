#include "ui.h"

#include <M5Cardputer.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "font8.h"
#if USB_MIDI_HOST
#include "midi_recorder.h"
#endif

UI ui;

static const char* NOTE_NAMES[12] = {"C", "C#", "D", "D#", "E", "F",
                                     "F#", "G", "G#", "A", "A#", "B"};

static void upperCopy(char* dst, const char* src, size_t n)
{
    size_t i = 0;
    for (; src[i] && i + 1 < n; ++i) {
        char c = src[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        dst[i] = c;
    }
    dst[i] = 0;
}

static void fmtTime(char* out, size_t n, uint32_t sec)
{
    if (sec >= 3600) snprintf(out, n, "%u:%02u:%02u", sec / 3600, (sec / 60) % 60, sec % 60);
    else             snprintf(out, n, "%u:%02u", sec / 60, sec % 60);
}

static void noteName(char* out, size_t n, uint8_t note)
{
    snprintf(out, n, "%s%d", NOTE_NAMES[note % 12], (int)note / 12 - 1);
}

// ---------------------------------------------------------------- primitives
void UI::begin()
{
    _c.setColorDepth(16);
    _c.createSprite(SCR_W, SCR_H);
    _c.fillSprite(C_BG);
}

int UI::tw(const char* s, int scale) const
{
    return (int)strlen(s) * 6 * scale;
}

void UI::text(int x, int y, const char* s, uint16_t col, bool bold, int scale)
{
    const uint8_t (*tbl)[FONT_H] = bold ? FONT8B : FONT8;
    int cx = x;
    for (const char* p = s; *p; ++p) {
        char c = *p;
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c < 32 || c > 95) c = '?';
        const uint8_t* g = tbl[c - 32];
        for (int row = 0; row < FONT_H; ++row) {
            uint8_t bits = g[row];
            for (int b = 0; b < FONT_W; ++b) {
                if (bits & (1 << b)) {
                    _c.fillRect(cx + b * scale, y + row * scale, scale, scale, col);
                }
            }
        }
        cx += 6 * scale;
        if (cx > SCR_W) break;
    }
}

void UI::textf(int x, int y, uint16_t col, bool bold, int scale, const char* fmt, ...)
{
    char buf[80];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    text(x, y, buf, col, bold, scale);
}

void UI::textR(int right, int y, const char* s, uint16_t col, bool bold, int scale)
{
    text(right - tw(s, scale), y, s, col, bold, scale);
}

void UI::textC(int cx, int y, const char* s, uint16_t col, bool bold, int scale)
{
    text(cx - tw(s, scale) / 2, y, s, col, bold, scale);
}

void UI::bar(int x, int y, int w, int h, float ratio, uint16_t fg, uint16_t bg)
{
    rect(x, y, w, h, bg);
    if (ratio < 0) ratio = 0;
    if (ratio > 1) ratio = 1;
    int fw = (int)((w - 2) * ratio);
    if (fw > 0) rect(x + 1, y + 1, fw, h - 2, fg);
}

// ---------------------------------------------------------------- icons
void UI::iconPlay(int x, int y, uint16_t col)
{
    _c.fillTriangle(x, y, x, y + 8, x + 7, y + 4, col);
}

void UI::iconPause(int x, int y, uint16_t col)
{
    rect(x, y, 2, 8, col);
    rect(x + 4, y, 2, 8, col);
}

void UI::iconPrev(int x, int y, uint16_t col)
{
    rect(x, y, 2, 8, col);
    _c.fillTriangle(x + 8, y, x + 2, y + 4, x + 8, y + 8, col);
}

void UI::iconNext(int x, int y, uint16_t col)
{
    _c.fillTriangle(x, y, x + 6, y + 4, x, y + 8, col);
    rect(x + 6, y, 2, 8, col);
}

void UI::iconVol(int x, int y, uint16_t col)
{
    _c.fillTriangle(x, y + 2, x + 3, y + 2, x + 6, y, col);
    _c.fillTriangle(x, y + 6, x + 3, y + 6, x + 6, y + 8, col);
    rect(x, y + 2, 3, 5, col);
    vline(x + 7, y + 2, 5, col);
    vline(x + 9, y, 9, col);
}

void UI::iconFolder(int x, int y, uint16_t col)
{
    frame(x, y + 1, 8, 6, col);
    rect(x, y, 4, 2, col);
}

void UI::iconNote(int x, int y, uint16_t col)
{
    rect(x + 4, y, 1, 7, col);
    rect(x + 4, y, 3, 1, col);
    rect(x + 6, y + 1, 1, 2, col);
    _c.fillEllipse(x + 2, y + 6, 3, 2, col);
}

void UI::iconEq(int x, int y, uint16_t col)
{
    rect(x, y + 4, 2, 4, col);
    rect(x + 3, y + 1, 2, 7, col);
    rect(x + 6, y + 3, 2, 5, col);
}

void UI::iconCard(int x, int y, uint16_t col)
{
    _c.drawLine(x, y, x + 17, y, col);
    _c.drawLine(x + 17, y, x + 23, y + 6, col);
    _c.drawLine(x + 23, y + 6, x + 23, y + 29, col);
    _c.drawLine(x + 23, y + 29, x, y + 29, col);
    _c.drawLine(x, y + 29, x, y, col);
    _c.drawLine(x + 17, y + 1, x + 17, y + 6, col);
    frame(x + 4, y + 9, 16, 12, col);
    text(x + 6, y + 12, "SD", col, true, 1);
}

void UI::iconBattery(int x, int y, uint8_t pct)
{
    frame(x, y, 12, 6, C_DIM);
    rect(x + 12, y + 2, 2, 2, C_DIM);
    int n = pct > 70 ? 8 : (pct > 35 ? 5 : 2);
    rect(x + 1, y + 1, n, 4, pct > 35 ? C_GREEN : C_RED);
}

// ---------------------------------------------------------------- chrome
void UI::topbar(const char* section)
{
    rect(0, 0, SCR_W, 13, C_BAR);
    hline(0, 12, SCR_W, C_LINE);

    // eighth note
    rect(9, 2, 1, 7, C_ACC);
    rect(9, 2, 3, 1, C_ACC);
    rect(11, 3, 1, 2, C_ACC);
    _c.fillEllipse(7, 8, 3, 2, C_ACC);

    text(17, 4, "MIDI", C_ACC, true, 1);
    vline(46, 3, 7, C_LINE);
    text(52, 4, section, C_FAINT, false, 1);

#if USB_MIDI_HOST
    uint16_t uc = !app.usbHostOk ? C_RED
                : (app.usbConnected ? C_AQUA
                : (app.usbDeviceSeen ? C_AMB : C_FAINT));
    text(156, 4, "USB", uc, true, 1);
#endif

    uint16_t sd = app.sdOk ? C_GREEN : C_RED;
    rect(185, 5, 5, 5, sd);
    text(193, 4, "SD", sd, true, 1);
    iconBattery(220, 4, app.battery);
}

void UI::hints(const char* k1, const char* a1, const char* k2, const char* a2,
               const char* k3, const char* a3)
{
    int w1 = tw(k1) + 6 + tw(a1);
    int w2 = k2 && k2[0] ? tw(k2) + 6 + tw(a2) : 0;
    int w3 = k3 && k3[0] ? tw(k3) + 6 + tw(a3) : 0;
    int total = w1 + (w2 ? 8 + w2 : 0) + (w3 ? 8 + w3 : 0);
    int x = (SCR_W - total) / 2;
    if (x < 3) x = 3;

    text(x, 127, k1, C_TXT, true, 1);
    text(x + tw(k1) + 6, 127, a1, C_DIM, false, 1);
    x += w1;
    if (w2) {
        x += 8;
        text(x, 127, k2, C_TXT, true, 1);
        text(x + tw(k2) + 6, 127, a2, C_DIM, false, 1);
        x += w2;
    }
    if (w3) {
        x += 8;
        text(x, 127, k3, C_TXT, true, 1);
        text(x + tw(k3) + 6, 127, a3, C_DIM, false, 1);
    }
}

void UI::badge(const char* label, uint16_t fg, uint16_t bg)
{
    int w = tw(label) + 8;
    int x = 235 - w;
    rect(x, 14, w, 11, bg);
    rect(x, 14, 2, 11, fg);
    text(x + 5, 17, label, fg, true, 1);
}

void UI::artTile()
{
    rect(4, 15, 36, 36, C_PAD);
    frame(4, 15, 36, 36, C_EDGE);
    for (int i = 0; i < 7; ++i) {
        int x = 8 + i * 4;
        rect(x, 27, 4, 13, C_TXT);
        if (i < 6) vline(x + 3, 27, 13, C_BG);
    }
    const int blk[5] = {0, 1, 3, 4, 5};
    for (int i = 0; i < 5; ++i) {
        int b = blk[i];
        rect(8 + (b + 1) * 4 - 2, 27, 3, 7, C_BG);
    }
    rect(8, 41, 28, 1, C_ACC);
}

void UI::keyboard(int x, int y, int whiteW, int h, int base, bool labels, bool demo)
{
    static const uint8_t semis[14] = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23};
    for (int i = 0; i < 14; ++i) {
        int note = base + semis[i];
        bool on = demo ? (note == 60 || note == 64 || note == 67)
                       : (!audio.paused() && !audio.ended() && audio.noteActive((uint8_t)note));
        int xk = x + i * whiteW;
        rect(xk, y, whiteW, h, on ? C_ACC : C_LT2);
        if (i < 13) vline(xk + whiteW - 1, y, h, C_BG);
    }

    const int blackAfter[10] = {0, 1, 3, 4, 5, 7, 8, 10, 11, 12};
    for (int i = 0; i < 10; ++i) {
        int w = blackAfter[i];
        int note = base + semis[w] + 1;
        bool on = demo ? false
                       : (!audio.paused() && !audio.ended() && audio.noteActive((uint8_t)note));
        int bw = 7;
        int bx = x + (w + 1) * whiteW - bw / 2;
        rect(bx, y, bw, h * 10 / 18, on ? C_ACC : C_BG);
    }

    if (labels) {
        char lb[6];
        int oct = base / 12 - 1;
        snprintf(lb, sizeof(lb), "C%d", oct);
        text(x + 2, y + h - 6, lb,
             (!audio.paused() && !audio.ended() && audio.noteActive((uint8_t)base)) ? C_BG : C_FAINT,
             false, 1);
        int n2 = base + 12;
        snprintf(lb, sizeof(lb), "C%d", oct + 1);
        text(x + 7 * whiteW + 2, y + h - 6, lb,
             (!audio.paused() && !audio.ended() && audio.noteActive((uint8_t)n2)) ? C_BG : C_FAINT,
             false, 1);
    }
}

void UI::liveRow()
{
    text(5, 79, "LIVE", C_FAINT, false, 1);

    char notes[28] = {0};
    int used = 0, total = 0;
    for (int n = 0; n < 128; ++n) {
        if (!audio.noteActive((uint8_t)n)) continue;
        ++total;
        if (used < 4) {
            char one[8];
            noteName(one, sizeof(one), (uint8_t)n);
            if (used) strncat(notes, " ", sizeof(notes) - strlen(notes) - 1);
            strncat(notes, one, sizeof(notes) - strlen(notes) - 1);
            ++used;
        }
    }
    if (total > 4) strncat(notes, "+", sizeof(notes) - strlen(notes) - 1);
    if (!total) strcpy(notes, "--");

    if (audio.paused()) {
        rect(36, 80, 2, 5, C_AMB);
        rect(40, 80, 2, 5, C_AMB);
        text(47, 79, notes, C_AMB, false, 1);
    } else if (audio.ended()) {
        text(35, 79, "IDLE", C_FAINT, false, 1);
    } else {
        text(35, 79, notes, C_TXT, false, 1);
    }

    bool ped = audio.sustain() && !audio.ended();
    frame(144, 77, 28, 11, ped ? C_AQUA : C_EDGE);
    text(149, 80, "PED", ped ? C_AQUA : C_FAINT, false, 1);

    text(181, 79, "VOX", C_FAINT, false, 1);
    textf(205, 79, C_TXT, false, 1, "%02u/24", (unsigned)audio.voices());
}

// ---------------------------------------------------------------- screens
void UI::drawLibrary()
{
    topbar("FILES");

    rect(0, 13, SCR_W, 12, C_PANEL);
    hline(0, 24, SCR_W, C_LINE);
    iconFolder(5, 16, C_AMB);

    char up[96];
    upperCopy(up, library.path(), sizeof(up));
    char path[40];
    if (strlen(up) > 26) snprintf(path, sizeof(path), "SD:...%s", up + strlen(up) - 23);
    else                 snprintf(path, sizeof(path), "SD:%s", up);
    text(17, 16, path, C_BLUE, true, 1);

    char count[16];
    snprintf(count, sizeof(count), "%d FILES", library.count());
    textR(235, 16, count, C_FAINT, false, 1);

    int n = library.count();
    for (int r = 0; r < 6; ++r) {
        int idx = app.scroll + r;
        if (idx >= n) break;
        int y = 26 + r * 16;
        bool sel = idx == app.sel;
        const FileEntry& e = library.at(idx);

        if (sel) rect(0, y, 236, 16, C_ACC);
        else     hline(0, y + 15, 236, C_LINE);

        uint16_t fg  = sel ? C_BG : C_TXT;
        uint16_t fg2 = sel ? C_BG : C_DIM;
        uint16_t fg3 = sel ? C_BG : C_FAINT;

        if (e.dir) {
            iconFolder(6, y + 4, sel ? C_BG : C_AMB);
        } else if (sel && idx == app.playIndex) {
            iconEq(6, y + 4, C_BG);
        } else {
            iconNote(6, y + 4, sel ? C_BG : C_BLUE);
        }

        char nm[27];
        strncpy(nm, e.name, sizeof(nm) - 1);
        nm[sizeof(nm) - 1] = 0;
        text(18, y + 5, nm, fg, sel, 1);

        if (e.dir) {
            textR(232, y + 5, "DIR", fg3, false, 1);
        } else {
            char dur[12];
            if (e.durSec) fmtTime(dur, sizeof(dur), e.durSec);
            else          strcpy(dur, "--:--");
            textR(206, y + 5, dur, fg2, false, 1);

            char sz[12];
            if (e.size >= 1024u * 1024u) snprintf(sz, sizeof(sz), "%uM", (unsigned)(e.size / (1024u * 1024u)));
            else                         snprintf(sz, sizeof(sz), "%uK", (unsigned)((e.size + 512) / 1024));
            textR(232, y + 5, sz, fg3, false, 1);
        }
    }

    if (n > 6) {
        rect(237, 26, 2, 96, C_PAD);
        int th = 96 * 6 / n;
        if (th < 8) th = 8;
        int ty = 26 + (96 - th) * app.scroll / (n - 6);
        rect(237, ty, 2, th, C_FAINT);
    }

    hints("D", "DELETE", "N", "RENAME", "BKSP", "UP");

    if (app.renaming && app.sel >= 0 && app.sel < library.count()) {
        rect(8, 30, 224, 70, C_BAR);
        frame(8, 30, 224, 70, C_AMB);
        textC(SCR_W / 2, 35, "RENAME FILE", C_AMB, true, 1);
        char oldn[26];
        strncpy(oldn, library.at(app.sel).name, sizeof(oldn) - 1);
        oldn[sizeof(oldn) - 1] = 0;
        textC(SCR_W / 2, 47, oldn, C_FAINT, false, 1);

        char nm[36];
        snprintf(nm, sizeof(nm), "%s%s", app.renameBuf, app.renameExt);
        textC(SCR_W / 2, 62, nm, C_TXT, true, 1);
        textC(SCR_W / 2, 78, "ENTER OK", C_AMB, true, 1);
        textC(SCR_W / 2, 89, "BKSP DEL   TAB NO", C_DIM, false, 1);
    }

    if (app.confirmDelete && app.sel >= 0 && app.sel < library.count()) {
        rect(10, 38, 220, 58, C_BAR);
        frame(10, 38, 220, 58, C_RED);
        textC(SCR_W / 2, 43, "DELETE FILE?", C_RED, true, 1);
        char nm[26];
        strncpy(nm, library.at(app.sel).name, sizeof(nm) - 1);
        nm[sizeof(nm) - 1] = 0;
        textC(SCR_W / 2, 56, nm, C_TXT, false, 1);
        textC(SCR_W / 2, 72, "ENTER YES", C_AMB, true, 1);
        textC(SCR_W / 2, 83, "BKSP NO", C_DIM, false, 1);
    }
}

void UI::drawPlayer()
{
    topbar("PLAYER");
    artTile();

    char t[23];
    strncpy(t, app.title[0] ? app.title : "UNTITLED", sizeof(t) - 1);
    t[sizeof(t) - 1] = 0;
    text(46, 16, t, C_TXT, true, 1);

    textf(46, 26, C_BLUE, false, 1, "%u TRACKS", (unsigned)app.info.tracks);

    int x = 46;
    text(x, 36, "SMF 1", C_FAINT, false, 1);
    x += tw("SMF 1") + 6;
    text(x, 36, "-", C_FAINT, false, 1);
    x += tw("-") + 6;
    char bpm[14];
    snprintf(bpm, sizeof(bpm), "%u.%u BPM", (unsigned)(app.info.bpmX10 / 10),
             (unsigned)(app.info.bpmX10 % 10));
    text(x, 36, bpm, C_DIM, false, 1);
    x += tw(bpm) + 6;
    text(x, 36, "-", C_FAINT, false, 1);
    x += tw("-") + 6;
    text(x, 36, app.info.key, C_DIM, false, 1);

    bool ended = audio.ended();
    bool paused = audio.paused();
    if (ended)        badge("ENDED", C_FAINT, C_PAD);
    else if (paused)  badge("PAUSED", C_AMB, C_AMB_DIM);
    else              badge("PLAYING", C_ACC, C_ACC_DIM);

    hline(0, 51, SCR_W, C_LINE);

    uint32_t pos = audio.positionUs() / 1000000u;
    uint32_t dur = audio.durationUs() / 1000000u;
    if (dur && pos > dur) pos = dur;
    char ts[12];
    fmtTime(ts, sizeof(ts), pos);
    text(5, 53, ts, ended ? C_DIM : C_TXT, true, 2);
    int slashX = 5 + tw(ts, 2) + 5;
    text(slashX, 58, "/", C_FAINT, false, 1);
    char total[12];
    fmtTime(total, sizeof(total), dur);
    text(slashX + 8, 58, total, C_DIM, false, 1);

    float ratio = audio.durationUs() ? (float)audio.positionUs() / (float)audio.durationUs() : 0;
    if (ratio > 1) ratio = 1;
    bar(5, 71, 230, 5, ratio, ended ? C_FAINT : (paused ? C_AMB : C_ACC));
    if (!ended) {
        int hx = 5 + (int)(230 * ratio);
        rect(hx - 1, 69, 3, 9, paused ? C_AMB : C_TXT);
    }

    uint32_t totalBars = 1;
    if (app.info.division && app.info.tsNum) {
        totalBars = (uint32_t)((uint64_t)app.info.maxTick * app.info.tsDen /
                               ((uint64_t)app.info.division * 4 * app.info.tsNum));
        if (totalBars == 0) totalBars = 1;
    }
    uint32_t barNo = (uint32_t)(ratio * totalBars) + 1;
    if (barNo > totalBars) barNo = totalBars;
    char barText[20];
    snprintf(barText, sizeof(barText), "BAR %u/%u", (unsigned)barNo, (unsigned)totalBars);
    textR(235, 58, barText, C_FAINT, false, 1);

    liveRow();
    keyboard(8, 89, 16, 18, 48, true, false);

    const int yb = 109;
    frame(5, yb, 18, 14, C_EDGE);
    iconPrev(9, yb + 3, C_TXT);
    rect(27, yb, 34, 14, C_ACC);
    if (paused || ended) iconPlay(41, yb + 3, C_BG);
    else                 iconPause(41, yb + 3, C_BG);
    frame(65, yb, 18, 14, C_EDGE);
    iconNext(70, yb + 3, C_TXT);

    frame(84, yb, 38, 14, app.loop ? C_ACC : C_EDGE);
    rect(88, yb + 5, 5, 5, app.loop ? C_ACC : C_BG);
    if (!app.loop) frame(88, yb + 5, 5, 5, C_FAINT);
    text(96, yb + 4, "LOOP", app.loop ? C_ACC : C_FAINT, false, 1);

    char spd[8];
#if USB_MIDI_HOST
    snprintf(spd, sizeof(spd), "%c%u", app.bpmFromFm1 ? 'F' : 'B',
             (unsigned)(app.bpmX10 / 10));
#else
    snprintf(spd, sizeof(spd), "x%.1f", audio.speed());
#endif
    text(126, yb + 4, spd, C_AQUA, true, 1);

    text(154, yb + 4, "VOL", C_FAINT, false, 1);
    iconVol(175, yb + 3, C_DIM);
    bar(188, yb + 5, 28, 4, app.volume / 100.0f, C_AQUA);
    char vol[6];
    snprintf(vol, sizeof(vol), "%u", (unsigned)app.volume);
    textR(236, yb + 4, vol, C_TXT, true, 1);

    if (ended)      hints("SPC", "REPLAY", "BKSP", "LIST", "", "");
    else if (paused) hints("SPC", "RESUME", "ARROWS", "SEEK", "BKSP", "LIST");
    else             hints("SPC", "PLAY/PAUSE", "ARROWS", "SEEK", "BKSP", "LIST");
}

void UI::drawRoll()
{
    topbar("PIANO");

    const int RX = 18;
    const int RY = 14;
    const int RW = SCR_W - RX;
    const int RH = 96;
    const int ROWS = 48;

    if (!app.ev || !app.evCount) {
        textC(SCR_W / 2, 55, "NO SONG", C_FAINT, true, 1);
        hline(0, 110, SCR_W, C_LINE);
        rect(0, 111, SCR_W, 24, C_BAR);
        textC(SCR_W / 2, 124, "TAB RECORDER", C_DIM);
        return;
    }

    uint32_t pos = audio.positionUs();
    uint32_t dur = audio.durationUs();
    int32_t  t0  = (int32_t)pos - 2000000;
    if (t0 < 0) t0 = 0;
    int32_t  t1  = t0 + 8000000;
    if (dur && t1 > (int32_t)dur) {
        t1 = (int32_t)dur;
        t0 = t1 - 8000000;
        if (t0 < 0) t0 = 0;
    }
    if (t1 <= t0) t1 = t0 + 1000;

    auto mapX = [&](int32_t t) -> int {
        int x = RX + (int)((int64_t)(t - t0) * RW / (t1 - t0));
        if (x < RX) x = RX;
        if (x > SCR_W - 1) x = SCR_W - 1;
        return x;
    };

    // Pass 1: visible note range for vertical auto-scroll.
    memset(_rollStart, 0xFF, sizeof(_rollStart));
    int  minN = 127, maxN = 0;
    bool any  = false;
    for (size_t i = 0; i < app.evCount; ++i) {
        const MidiEvent& e = app.ev[i];
        if (e.t > (uint32_t)t1) break;
        int ch = e.pad & 15;
        if (e.type == EV_ON) {
            _rollStart[ch][e.note] = (int32_t)e.t;
        } else if (e.type == EV_OFF) {
            int32_t s = _rollStart[ch][e.note];
            if (s >= 0) {
                if ((int32_t)e.t >= t0) {
                    if (e.note < minN) minN = e.note;
                    if (e.note > maxN) maxN = e.note;
                    any = true;
                }
                _rollStart[ch][e.note] = -1;
            }
        }
    }
    for (int ch = 0; ch < 16; ++ch)
        for (int n = 0; n < 128; ++n)
            if (_rollStart[ch][n] >= 0) {
                if (n < minN) minN = n;
                if (n > maxN) maxN = n;
                any = true;
            }

    int target = 48;
    if (any) {
        target = (minN + maxN) / 2 - ROWS / 2;
        if (target < 0) target = 0;
        if (target > 127 - ROWS) target = 127 - ROWS;
    }
    int d = target - _rollBase;
    if (d > 4) d = 4;
    if (d < -4) d = -4;
    _rollBase += d;

    // Background rows + keyboard.
    rect(0, RY, RX, RH, C_PANEL);
    rect(RX, RY, RW, RH, C_BG);
    for (int row = 0; row < ROWS; ++row) {
        int note = _rollBase + ROWS - 1 - row;
        int y    = RY + row * 2;
        int pc   = note % 12;
        bool black = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);

        if (black) rect(RX, y, RW, 2, C_BAR);
        if (pc == 0) hline(RX, y + 1, RW, C_LINE);

        if (black) {
            rect(0, y, 11, 2, C_BG);
            rect(11, y, RX - 11, 2, C_LT2);
        } else {
            rect(0, y, RX, 2, C_LT2);
            hline(0, y + 1, RX, C_EDGE);
        }
    }

    // Beat / bar lines.
    uint32_t beat = app.info.bpmX10 ? (600000000u / app.info.bpmX10) : 500000u;
    if (beat < 100000u) beat = 100000u;
    uint32_t barLen = beat * 4;
    int64_t first = ((int64_t)t0 / beat) * beat;
    if (first < t0) first += beat;
    for (int64_t t = first; t < t1; t += beat) {
        int x = mapX((int32_t)t);
        vline(x, RY, RH, ((uint32_t)t % barLen) == 0 ? C_EDGE : C_LINE);
    }

    // Pass 2: notes.
    memset(_rollStart, 0xFF, sizeof(_rollStart));
    auto drawNote = [&](int32_t s, int32_t e, uint8_t note, uint8_t ch) {
        if (e <= t0 || s >= t1) return;
        int32_t a = s < t0 ? t0 : s;
        int32_t b = e > t1 ? t1 : e;
        if (b <= a) return;
        int x0 = mapX(a);
        int x1 = mapX(b);
        if (x1 - x0 < 2) x1 = x0 + 2;
        if (x1 > SCR_W) x1 = SCR_W;
        if (x0 >= SCR_W) return;

        int row = _rollBase + ROWS - 1 - note;
        if (row < 0 || row >= ROWS) return;
        int y = RY + row * 2;

        bool active = (s <= (int32_t)pos && e > (int32_t)pos);
        uint16_t col;
        if (active)          col = C_ACC;
        else if (e <= (int32_t)pos) col = C_FAINT;
        else                 col = (ch & 1) ? C_BLUE : C_AQUA;

        rect(x0, y, x1 - x0, 2, col);
    };

    for (size_t i = 0; i < app.evCount; ++i) {
        const MidiEvent& e = app.ev[i];
        if (e.t > (uint32_t)t1) break;
        int ch = e.pad & 15;
        if (e.type == EV_ON) {
            _rollStart[ch][e.note] = (int32_t)e.t;
        } else if (e.type == EV_OFF) {
            int32_t s = _rollStart[ch][e.note];
            if (s >= 0) {
                drawNote(s, (int32_t)e.t, e.note, (uint8_t)ch);
                _rollStart[ch][e.note] = -1;
            }
        }
    }
    for (int ch = 0; ch < 16; ++ch)
        for (int n = 0; n < 128; ++n)
            if (_rollStart[ch][n] >= 0)
                drawNote(_rollStart[ch][n], t1, (uint8_t)n, (uint8_t)ch);

    // Playhead.
    int px = mapX((int32_t)pos);
    vline(px, RY, RH, C_TXT);
    _c.fillTriangle(px - 2, RY, px + 2, RY, px, RY + 4, C_ACC);

    // Footer / transport status.
    hline(0, 110, SCR_W, C_LINE);
    rect(0, 111, SCR_W, 24, C_BAR);

    char a[16], b[16], tm[36];
    fmtTime(a, sizeof(a), pos / 1000000u);
    fmtTime(b, sizeof(b), dur / 1000000u);
    snprintf(tm, sizeof(tm), "%s / %s", a, b);
    text(6, 113, tm, C_TXT, true, 1);

    if (app.loop) textR(136, 113, "LOOP", C_AMB, true, 1);

    char bpm[16];
    snprintf(bpm, sizeof(bpm), "%u BPM", (unsigned)(app.info.bpmX10 / 10));
    textR(204, 113, bpm, C_DIM, false, 1);

    char spd[8];
#if USB_MIDI_HOST
    snprintf(spd, sizeof(spd), "%c%u", app.bpmFromFm1 ? 'F' : 'B',
             (unsigned)(app.bpmX10 / 10));
#else
    snprintf(spd, sizeof(spd), "x%.1f", audio.speed());
#endif
    textR(234, 113, spd, C_AQUA, true, 1);

    textC(SCR_W / 2, 124, "[ ] SEEK   TAB RECORDER", C_DIM);
}

void UI::drawInput()
{
    topbar("RECORDER");

#if USB_MIDI_HOST
    const int RX = 18;
    const int RY = 28;
    const int RW = SCR_W - RX;
    const int RH = 72;
    const int ROWS = 36;

    // Channel selector: CH1 / CH2 / CH3 / CH10 (0 = CH10).
    auto chip = [&](int x, const char* label, uint8_t ch) {
        bool on = rec.channelOn(ch);
        if (on) {
            rect(x, 15, 48, 11, C_ACC);
            text(x + 7, 17, label, C_BG, true, 1);
        } else {
            frame(x, 15, 48, 11, C_EDGE);
            text(x + 7, 17, label, C_DIM, false, 1);
        }
    };
    chip(4, "CH1", 0);
    chip(58, "CH2", 1);
    chip(112, "CH3", 2);
    chip(166, "CH10", 9);

    if (!app.usbConnected) {
        textC(SCR_W / 2, 55, "CONNECT FM1 USB", C_AMB, true, 1);
        textC(SCR_W / 2, 68, "HOST BUILD + USB-C CABLE", C_FAINT, false, 1);
        hline(0, 100, SCR_W, C_LINE);
        rect(0, 101, SCR_W, 34, C_BAR);
        textC(SCR_W / 2, 124, "TAB NEXT   BKSP BACK", C_DIM);
        return;
    }

    uint32_t t1 = rec.active() ? rec.elapsedUs() : rec.durationUs();
    if (t1 < 1000) t1 = 1000;
    int32_t t0 = (int32_t)t1 - 8000000;
    if (t0 < 0) t0 = 0;
    if (t1 <= (uint32_t)t0) t1 = t0 + 1000;

    auto mapX = [&](int32_t t) -> int {
        int x = RX + (int)((int64_t)(t - t0) * RW / ((int32_t)t1 - t0));
        if (x < RX) x = RX;
        if (x > SCR_W - 1) x = SCR_W - 1;
        return x;
    };

    // Visible note range.
    memset(_rollStart, 0xFF, sizeof(_rollStart));
    int minN = 127, maxN = 0;
    bool any = false;
    const RecEvent* ev = rec.events();
    int n = rec.count();
    for (int i = 0; i < n; ++i) {
        const RecEvent& e = ev[i];
        if (!rec.channelOn(e.ch)) continue;
        if (e.t > (uint32_t)t1) break;
        if (e.type == REC_ON) {
            _rollStart[e.ch][e.note] = (int32_t)e.t;
        } else if (e.type == REC_OFF) {
            int32_t s = _rollStart[e.ch][e.note];
            if (s >= 0) {
                if ((int32_t)e.t >= t0) {
                    if (e.note < minN) minN = e.note;
                    if (e.note > maxN) maxN = e.note;
                    any = true;
                }
                _rollStart[e.ch][e.note] = -1;
            }
        }
    }
    for (int ch = 0; ch < 16; ++ch)
        for (int note = 0; note < 128; ++note)
            if (_rollStart[ch][note] >= 0) {
                if (note < minN) minN = note;
                if (note > maxN) maxN = note;
                any = true;
            }

    int target = 48;
    if (any) {
        target = (minN + maxN) / 2 - ROWS / 2;
        if (target < 0) target = 0;
        if (target > 127 - ROWS) target = 127 - ROWS;
    }
    int d = target - _rollBase;
    if (d > 4) d = 4;
    if (d < -4) d = -4;
    _rollBase += d;

    // Background + keyboard.
    rect(0, RY, RX, RH, C_PANEL);
    rect(RX, RY, RW, RH, C_BG);
    for (int row = 0; row < ROWS; ++row) {
        int note = _rollBase + ROWS - 1 - row;
        int y = RY + row * 2;
        int pc = note % 12;
        bool black = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
        if (black) rect(RX, y, RW, 2, C_BAR);
        if (pc == 0) hline(RX, y + 1, RW, C_LINE);
        if (black) {
            rect(0, y, 11, 2, C_BG);
            rect(11, y, RX - 11, 2, C_LT2);
        } else {
            rect(0, y, RX, 2, C_LT2);
            hline(0, y + 1, RX, C_EDGE);
        }
    }

    // One-second grid, 4-second bars.
    int32_t first = (t0 / 1000000) * 1000000;
    if (first < t0) first += 1000000;
    for (int32_t t = first; t < (int32_t)t1; t += 1000000) {
        int x = mapX(t);
        vline(x, RY, RH, (t % 4000000) == 0 ? C_EDGE : C_LINE);
    }

    // Notes.
    memset(_rollStart, 0xFF, sizeof(_rollStart));
    auto drawNote = [&](int32_t s, int32_t e, uint8_t note, uint8_t ch) {
        if (e <= t0 || s >= (int32_t)t1) return;
        int32_t a = s < t0 ? t0 : s;
        int32_t b = e > (int32_t)t1 ? (int32_t)t1 : e;
        if (b <= a) return;
        int x0 = mapX(a);
        int x1 = mapX(b);
        if (x1 - x0 < 2) x1 = x0 + 2;
        if (x1 > SCR_W) x1 = SCR_W;
        if (x0 >= SCR_W) return;
        int row = _rollBase + ROWS - 1 - note;
        if (row < 0 || row >= ROWS) return;
        int y = RY + row * 2;
        uint16_t col = (ch == 0) ? C_ACC : (ch == 1) ? C_AQUA : (ch == 2) ? C_AMB : C_BLUE;
        rect(x0, y, x1 - x0, 2, col);
    };

    for (int i = 0; i < n; ++i) {
        const RecEvent& e = ev[i];
        if (!rec.channelOn(e.ch)) continue;
        if (e.t > t1) break;
        if (e.type == REC_ON) {
            _rollStart[e.ch][e.note] = (int32_t)e.t;
        } else if (e.type == REC_OFF) {
            int32_t s = _rollStart[e.ch][e.note];
            if (s >= 0) {
                drawNote(s, (int32_t)e.t, e.note, e.ch);
                _rollStart[e.ch][e.note] = -1;
            }
        }
    }
    for (int ch = 0; ch < 16; ++ch)
        for (int note = 0; note < 128; ++note)
            if (_rollStart[ch][note] >= 0)
                drawNote(_rollStart[ch][note], (int32_t)t1, (uint8_t)note, (uint8_t)ch);

    // Playhead.
    int px = SCR_W - 3;
    vline(px, RY, RH, C_TXT);
    _c.fillTriangle(px - 2, RY, px + 2, RY, px, RY + 4, C_ACC);

    if (rec.recording() && rec.armed()) {
        textC(SCR_W / 2, 54, "WAIT MIDI...", C_AMB, true, 1);
        textC(SCR_W / 2, 67, "FM1 OUT = SEQ", C_FAINT, false, 1);
    } else if (n == 0 && !rec.recording()) {
        textC(SCR_W / 2, 58, "PRESS R TO RECORD", C_FAINT, false, 1);
    }

    // Status.
    hline(0, 100, SCR_W, C_LINE);
    rect(0, 101, SCR_W, 34, C_BAR);

    char tm[16];
    fmtTime(tm, sizeof(tm), t1 / 1000000u);
    if (rec.recording()) {
        text(6, 103, "REC", C_RED, true, 1);
    } else if (rec.active()) {
        text(6, 103, "IN", C_AQUA, true, 1);
    } else {
        text(6, 103, "TAKE", C_DIM, false, 1);
    }
    text(42, 103, tm, C_TXT, false, 1);

    char evn[16];
    snprintf(evn, sizeof(evn), "EV %d", n);
    text(100, 103, evn, C_DIM, false, 1);

    char bpm[16];
    snprintf(bpm, sizeof(bpm), "%u BPM", (unsigned)(rec.bpmX10() / 10));
    textR(234, 103, bpm, C_DIM, false, 1);

    textC(SCR_W / 2, 124, "R REC/STOP  1/2/3/4 CH  TAB NEXT", C_DIM);
#else
    textC(SCR_W / 2, 55, "USB HOST BUILD ONLY", C_FAINT, true, 1);
    hline(0, 100, SCR_W, C_LINE);
    rect(0, 101, SCR_W, 34, C_BAR);
    textC(SCR_W / 2, 124, "TAB NEXT", C_DIM);
#endif
}

void UI::drawNoFiles()
{
    topbar("FILES");
    rect(0, 13, SCR_W, 12, C_PANEL);
    hline(0, 24, SCR_W, C_LINE);
    iconFolder(5, 16, C_AMB);
    char up[96], path[40];
    upperCopy(up, library.path(), sizeof(up));
    snprintf(path, sizeof(path), "SD:%s", up);
    text(17, 16, path, C_BLUE, true, 1);
    textR(235, 16, "0 FILES", C_FAINT, false, 1);

    rect(98, 30, 18, 7, C_AMB);
    frame(98, 36, 44, 32, C_AMB);
    hline(99, 42, 42, C_AMB);

    textC(120, 78, "NO MIDI FILES", C_TXT, true, 2);
    textC(120, 96, "PUT .MID FILES IN SD:/MIDI", C_DIM, false, 1);

    frame(72, 107, 96, 13, C_ACC);
    textC(120, 111, "RESCAN  ENTER", C_ACC, true, 1);

    hints("ENTER", "RESCAN", "BKSP", "ROOT", "", "");
}

void UI::drawNoSd()
{
    topbar("FILES");
    rect(0, 13, SCR_W, 12, C_PANEL);
    hline(0, 24, SCR_W, C_LINE);
    iconFolder(5, 16, C_FAINT);
    text(17, 16, "SD:/", C_FAINT, true, 1);
    textR(235, 16, "-- FILES", C_FAINT, false, 1);

    iconCard(104, 32, C_RED);

    textC(120, 72, "NO SD CARD", C_RED, true, 2);
    textC(120, 91, "INSERT CARD, THEN PRESS ENTER", C_DIM, false, 1);

    frame(80, 105, 80, 13, C_RED);
    textC(120, 109, "RETRY  ENTER", C_RED, true, 1);

    hints("ENTER", "RETRY", "BKSP", "EXIT", "", "");
}

// ---------------------------------------------------------------- boot
void UI::boot(uint8_t pct, int files)
{
    _c.fillSprite(C_BG);
    text(5, 6, "M5STACK - CARDPUTER-ADV", C_FAINT, false, 1);
    textR(235, 6, "V" APP_VERSION, C_FAINT, false, 1);

    textC(120, 24, "MIDI", C_TXT, true, 4);
    textC(120, 51, "PIANO PLAYER", C_ACC, true, 1);
    hline(96, 64, 48, C_LINE);

    textC(120, 76, "SCANNING SD CARD", C_DIM, false, 1);
    bar(70, 88, 100, 4, pct / 100.0f, C_ACC);
    textf(120 - tw("00 FILES FOUND") / 2, 98, C_FAINT, false, 1, "%d FILES FOUND", files);

    rect(0, 109, SCR_W, 2, C_ACC);
    keyboard(1, 111, 17, 23, 48, false, true);

    _c.pushSprite(0, 0);
}

void UI::loading(const char* name)
{
    _c.fillSprite(C_BG);
    textC(120, 48, "LOADING", C_TXT, true, 2);
    char up[32];
    upperCopy(up, name, sizeof(up));
    textC(120, 72, up, C_DIM, false, 1);
    rect(70, 88, 100, 4, C_LINE);
    rect(71, 89, 98, 2, C_ACC);
    _c.pushSprite(0, 0);
}

// ---------------------------------------------------------------- render
void UI::render()
{
    _c.fillSprite(C_BG);
    switch (app.screen) {
        case Screen::LIBRARY:  drawLibrary(); break;
        case Screen::PLAYER:   drawPlayer();  break;
        case Screen::ROLL:     drawRoll();    break;
        case Screen::MIDI_IN:    drawInput();   break;
        case Screen::NO_FILES: drawNoFiles(); break;
        case Screen::NO_SD:    drawNoSd();    break;
        default: break;
    }

    if (app.toast[0] && (int32_t)(millis() - app.toastUntil) < 0) {
        int w = tw(app.toast) + 16;
        int x = (SCR_W - w) / 2;
        int y = app.screen == Screen::PLAYER ? 94 : 108;
        rect(x, y, w, 13, C_PAD);
        frame(x, y, w, 13, C_EDGE);
        textC(120, y + 4, app.toast, C_RED, false, 1);
    }

    _c.pushSprite(0, 0);
}
