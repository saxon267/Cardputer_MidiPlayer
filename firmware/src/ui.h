#pragma once
#include <M5Cardputer.h>
#include <M5GFX.h>
#include "app.h"

class UI {
public:
    void begin();
    void boot(uint8_t pct, int files);
    void loading(const char* text);
    void render();

private:
    M5Canvas _c{&M5Cardputer.Display};

    void text(int x, int y, const char* s, uint16_t col, bool bold = false, int scale = 1);
    void textf(int x, int y, uint16_t col, bool bold, int scale, const char* fmt, ...);
    void textR(int right, int y, const char* s, uint16_t col, bool bold = false, int scale = 1);
    void textC(int cx, int y, const char* s, uint16_t col, bool bold = false, int scale = 1);
    int  tw(const char* s, int scale = 1) const;

    void rect(int x, int y, int w, int h, uint16_t col) { _c.fillRect(x, y, w, h, col); }
    void frame(int x, int y, int w, int h, uint16_t col) { _c.drawRect(x, y, w, h, col); }
    void hline(int x, int y, int w, uint16_t col) { _c.drawFastHLine(x, y, w, col); }
    void vline(int x, int y, int h, uint16_t col) { _c.drawFastVLine(x, y, h, col); }
    void bar(int x, int y, int w, int h, float ratio, uint16_t fg, uint16_t bg = C_LINE);

    void iconPlay(int x, int y, uint16_t col);
    void iconPause(int x, int y, uint16_t col);
    void iconPrev(int x, int y, uint16_t col);
    void iconNext(int x, int y, uint16_t col);
    void iconVol(int x, int y, uint16_t col);
    void iconFolder(int x, int y, uint16_t col);
    void iconNote(int x, int y, uint16_t col);
    void iconEq(int x, int y, uint16_t col);
    void iconCard(int x, int y, uint16_t col);
    void iconBattery(int x, int y, uint8_t pct);

    void topbar(const char* section);
    void hints(const char* k1, const char* a1, const char* k2, const char* a2,
               const char* k3, const char* a3);
    void badge(const char* label, uint16_t fg, uint16_t bg);
    void keyboard(int x, int y, int whiteW, int h, int base, bool labels, bool highlightDemo);
    void artTile();
    void liveRow();

    void drawLibrary();
    void drawPlayer();
    void drawRoll();
    void drawInput();
    void drawNoFiles();
    void drawNoSd();

    int32_t _rollStart[16][128];
    int     _rollBase = 48;
};

extern UI ui;
