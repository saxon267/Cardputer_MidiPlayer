#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct LovyanGFX {};

inline const char* g_ui_out = "ui.bmp";

class M5Canvas {
public:
    explicit M5Canvas(LovyanGFX* p = nullptr) : _p(p) {}
    ~M5Canvas() { delete[] _buf; }

    void setColorDepth(int) {}
    void createSprite(int w, int h)
    {
        _w = w;
        _h = h;
        delete[] _buf;
        _buf = new uint16_t[w * h];
    }
    void fillSprite(uint16_t c)
    {
        for (int i = 0; i < _w * _h; ++i) _buf[i] = c;
    }

    void fillRect(int x, int y, int w, int h, uint16_t c)
    {
        if (w <= 0 || h <= 0) return;
        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }
        if (x + w > _w) w = _w - x;
        if (y + h > _h) h = _h - y;
        for (int yy = y; yy < y + h; ++yy)
            for (int xx = x; xx < x + w; ++xx) _buf[yy * _w + xx] = c;
    }

    void drawRect(int x, int y, int w, int h, uint16_t c)
    {
        drawFastHLine(x, y, w, c);
        drawFastHLine(x, y + h - 1, w, c);
        drawFastVLine(x, y, h, c);
        drawFastVLine(x + w - 1, y, h, c);
    }

    void drawFastHLine(int x, int y, int w, uint16_t c)
    {
        if (y < 0 || y >= _h) return;
        if (x < 0) { w += x; x = 0; }
        if (x + w > _w) w = _w - x;
        for (int i = 0; i < w; ++i) _buf[y * _w + x + i] = c;
    }

    void drawFastVLine(int x, int y, int h, uint16_t c)
    {
        if (x < 0 || x >= _w) return;
        if (y < 0) { h += y; y = 0; }
        if (y + h > _h) h = _h - y;
        for (int i = 0; i < h; ++i) _buf[(y + i) * _w + x] = c;
    }

    void drawLine(int x0, int y0, int x1, int y1, uint16_t c)
    {
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (;;) {
            if (x0 >= 0 && x0 < _w && y0 >= 0 && y0 < _h) _buf[y0 * _w + x0] = c;
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    static int64_t edge(int ax, int ay, int bx, int by, int px, int py)
    {
        return (int64_t)(px - ax) * (by - ay) - (int64_t)(py - ay) * (bx - ax);
    }

    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c)
    {
        int minX = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
        int maxX = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
        int minY = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
        int maxY = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
        int64_t area = edge(x0, y0, x1, y1, x2, y2);
        if (area == 0) return;
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                int64_t w0 = edge(x1, y1, x2, y2, x, y);
                int64_t w1 = edge(x2, y2, x0, y0, x, y);
                int64_t w2 = edge(x0, y0, x1, y1, x, y);
                if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                    if (x >= 0 && x < _w && y >= 0 && y < _h) _buf[y * _w + x] = c;
                }
            }
        }
    }

    void fillEllipse(int cx, int cy, int rx, int ry, uint16_t c)
    {
        if (rx <= 0 || ry <= 0) return;
        for (int y = -ry; y <= ry; ++y) {
            int span = (int)((int64_t)rx * rx * (1.0 - (double)y * y / ((double)ry * ry)));
            int x = 0;
            while ((int64_t)(x + 1) * (x + 1) <= (int64_t)span) ++x;
            drawFastHLine(cx - x, cy + y, 2 * x + 1, c);
        }
    }

    void pushSprite(int x, int y);

private:
    LovyanGFX* _p;
    int _w = 0;
    int _h = 0;
    uint16_t* _buf = nullptr;
};

inline void M5Canvas::pushSprite(int x, int y)
{
    (void)x;
    (void)y;
    if (!_buf) return;
    FILE* f = fopen(g_ui_out, "wb");
    if (!f) return;
    int rowBytes = (_w * 3 + 3) & ~3;
    uint32_t dataSize = (uint32_t)rowBytes * _h;
    uint8_t hdr[54] = {0};
    hdr[0] = 'B'; hdr[1] = 'M';
    uint32_t total = 54 + dataSize;
    memcpy(hdr + 2, &total, 4);
    uint32_t off = 54;
    memcpy(hdr + 10, &off, 4);
    uint32_t dib = 40;
    memcpy(hdr + 14, &dib, 4);
    int32_t w = _w, h = -_h;  // top-down
    memcpy(hdr + 18, &w, 4);
    memcpy(hdr + 22, &h, 4);
    uint16_t planes = 1, bpp = 24;
    memcpy(hdr + 26, &planes, 2);
    memcpy(hdr + 28, &bpp, 2);
    memcpy(hdr + 34, &dataSize, 4);
    fwrite(hdr, 1, 54, f);

    uint8_t* row = new uint8_t[rowBytes];
    for (int yy = 0; yy < _h; ++yy) {
        memset(row, 0, rowBytes);
        for (int xx = 0; xx < _w; ++xx) {
            uint16_t c = _buf[yy * _w + xx];
            uint8_t r = (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
            uint8_t g = (uint8_t)(((c >> 5) & 0x3F) * 255 / 63);
            uint8_t b = (uint8_t)((c & 0x1F) * 255 / 31);
            row[xx * 3 + 0] = b;
            row[xx * 3 + 1] = g;
            row[xx * 3 + 2] = r;
        }
        fwrite(row, 1, rowBytes, f);
    }
    delete[] row;
    fclose(f);
}
