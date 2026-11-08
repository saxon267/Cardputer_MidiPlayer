# -*- coding: utf-8 -*-
"""Resource-budget infographic for the Cardputer-Adv MIDI player."""

import os

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS = os.path.join(HERE, "..", "design", "fonts")
OUT = os.path.join(HERE, "midi-budget.png")

W, H = 1280, 760

BG = (29, 32, 33)
BAR = (40, 40, 40)
PADBG = (50, 48, 47)
EDGE = (80, 73, 69)
LINE = (60, 56, 54)
TXT = (235, 219, 178)
DIM = (168, 153, 132)
FAINT = (124, 111, 100)
ACC = (254, 128, 25)
AMB = (250, 189, 47)
AQUA = (142, 192, 124)
BLUE = (131, 165, 152)
PURPLE = (211, 134, 155)
RED = (251, 73, 52)
GREEN = (184, 187, 38)

f8 = ImageFont.truetype(os.path.join(FONTS, "Silkscreen-Regular.ttf"), 8)
f8b = ImageFont.truetype(os.path.join(FONTS, "Silkscreen-Bold.ttf"), 8)
f16 = ImageFont.truetype(os.path.join(FONTS, "Silkscreen-Regular.ttf"), 16)
f16b = ImageFont.truetype(os.path.join(FONTS, "Silkscreen-Bold.ttf"), 16)
f32b = ImageFont.truetype(os.path.join(FONTS, "Silkscreen-Bold.ttf"), 32)


def txt(d, x, y, s, font=f8, fill=TXT):
    off = {8: -4, 16: -7, 32: -13}[font.size]
    d.text((x, y + off), s, font=font, fill=fill)


def tw(s, font=f8):
    return int(font.getlength(s))


def tr(d, right, y, s, font=f8, fill=TXT):
    txt(d, right - tw(s, font), y, s, font, fill)


def rect(d, x, y, w, h, fill=None, outline=None):
    d.rectangle([x, y, x + w - 1, y + h - 1], fill=fill, outline=outline)


def hline(d, x, y, w, fill=LINE):
    d.line([x, y, x + w - 1, y], fill=fill)


def stack(d, x, y, w, h, parts, total):
    cur = x
    for i, (name, kb, col, note) in enumerate(parts):
        seg = int(round(w * kb / total))
        if i == len(parts) - 1:
            seg = x + w - cur
        if seg <= 0:
            continue
        rect(d, cur, y, seg, h, col)
        if seg > 4 and i < len(parts) - 1:
            d.line([cur + seg - 1, y, cur + seg - 1, y + h - 1], fill=BG)
        cur += seg
    rect(d, x, y, w, h, None, LINE)


def legend(d, x, y, items, cols, colw):
    for i, (name, kb, col, note) in enumerate(items):
        c, r = i % cols, i // cols
        xx = x + c * colw
        yy = y + r * 22
        rect(d, xx, yy + 2, 7, 7, col)
        txt(d, xx + 12, yy + 2, name, f8b, TXT)
        vx = xx + 12 + tw(name, f8b) + 6
        txt(d, vx, yy + 2, f"{kb} KB", f8, DIM)
        if note:
            txt(d, vx + tw(f"{kb} KB", f8) + 8, yy + 2, note, f8, FAINT)


def main():
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)
    d.fontmode = "1"

    txt(d, 60, 26, "RESOURCE BUDGET", f32b, TXT)
    txt(d, 60, 60, "CARDPUTER-ADV  ·  v0.1 FIRMWARE  ·  24 VOICES  ·  44.1 kHz MONO", f8, FAINT)
    hline(d, 60, 82, W - 120, ACC)

    # ---------------------------------------------------------------- SRAM
    y = 108
    txt(d, 60, y, "SRAM 512 KB", f16b, TXT)
    tr(d, W - 60, y + 4, "USED 334 KB  ·  FREE 178 KB", f8, DIM)
    bx, by, bw, bh = 60, y + 28, W - 120, 44
    ram = [
        ("FRAMEWORK", 90, EDGE, ""),
        ("SPRITE", 65, BLUE, "240×135×16bpp"),
        ("FILE LIST", 16, DIM, "64 entries"),
        ("EVENTS", 112, PURPLE, "14000 × 8 B"),
        ("PARSER", 2, AQUA, "tempo map / transport"),
        ("AUDIO", 3, ACC, "6 × 128 frames"),
        ("VOICES", 2, AMB, "24 voices + sine"),
        ("STACKS", 14, FAINT, "audio / ui / sd"),
        ("OTHER", 30, GREEN, "NVS / heap"),
        ("FREE", 178, PADBG, ""),
    ]
    stack(d, bx, by, bw, bh, ram, 512)
    # labels for the wide segments
    txt(d, bx + 8, by + 12, "FRAMEWORK 90", f8b, TXT)
    txt(d, bx + 8, by + 24, "18%", f8, FAINT)
    # sprite label
    sx = bx + int(bw * 90 / 512)
    txt(d, sx + 6, by + 12, "SPRITE 65", f8b, BG)
    txt(d, sx + 6, by + 24, "13%", f8, BG)
    # events label
    ex = bx + int(bw * 171 / 512)
    txt(d, ex + 8, by + 12, "EVENTS 112", f8b, BG)
    txt(d, ex + 8, by + 24, "22%  ·  14K", f8, BG)
    # free label
    fx = bx + int(bw * 334 / 512)
    txt(d, fx + 8, by + 12, "FREE 178 KB", f8b, TXT)
    txt(d, fx + 8, by + 24, "35%", f8, FAINT)

    legend(d, 60, by + bh + 18, ram, 3, 386)

    # ---------------------------------------------------------------- FLASH
    y = 330
    txt(d, 60, y, "PHASE 2 · 4 MB PIANO PARTITION", f16b, TXT)
    tr(d, W - 60, y + 4, "APP 3 MB + STORAGE 0.9 MB LIVE IN THE OTHER 4 MB", f8, DIM)
    bx, by, bw, bh = 60, y + 28, W - 120, 44
    flash = [
        ("SAMPLES", 3104, AQUA, "18 zones × 4 s × 22.05 kHz · 16-bit"),
        ("META", 8, DIM, "zone table / loop points"),
        ("FREE", 984, PADBG, "headroom: longer tails or 2nd layer"),
    ]
    stack(d, bx, by, bw, bh, flash, 4096)
    px = bx + 8
    txt(d, px + 8, by + 12, "SAMPLES 3.0 MB", f8b, BG)
    txt(d, px + 8, by + 24, "76%  ·  18 ZONES", f8, BG)
    fx = bx + int(bw * 3112 / 4096)
    txt(d, fx + 8, by + 12, "FREE 1.0 MB", f8b, TXT)
    txt(d, fx + 8, by + 24, "24%", f8, FAINT)

    legend(d, 60, by + bh + 18, flash, 3, 386)

    # ---------------------------------------------------------------- LATENCY
    y = 530
    txt(d, 60, y, "AUDIO LATENCY  ~20 ms", f16b, TXT)
    tr(d, W - 60, y + 4, "PLAY / PAUSE FEELS INSTANT", f8, DIM)
    bx, by, bw, bh = 60, y + 28, W - 120, 34
    lat = [
        ("KEY", 0.5, BLUE),
        ("UI", 1.5, DIM),
        ("BLOCK + DMA", 17, ACC),
        ("CODEC + AMP", 2, AQUA),
    ]
    total = 21.0
    cur = bx
    for i, (name, ms, col) in enumerate(lat):
        seg = int(round(bw * ms / total))
        if i == len(lat) - 1:
            seg = bx + bw - cur
        rect(d, cur, by, seg, bh, col)
        if seg > 90:
            txt(d, cur + 8, by + 13, f"{name} {ms:g} ms", f8b, BG)
        cur += seg
    rect(d, bx, by, bw, bh, None, LINE)

    # callouts under the bar
    cols = [(60, "KEY 0.5 ms", "TCA8418 interrupt → event"),
            (330, "UI 1.5 ms", "state → audio queue"),
            (600, "BLOCK + DMA 17 ms", "6 × 128 frames @ 44.1 kHz"),
            (900, "CODEC + AMP 2 ms", "ES8311 + NS4150B")]
    for x, name, note in cols:
        rect(d, x, by + bh + 14, 6, 6, ACC)
        txt(d, x + 11, by + bh + 12, name, f8b, TXT)
        txt(d, x + 11, by + bh + 23, note, f8, FAINT)

    # footer
    hline(d, 60, 686, W - 120, LINE)
    txt(d, 60, 700, "24 VOICES  ·  44.1 kHz MONO  ·  22.05 kHz SAMPLES (PHASE 2)  ·  7–10 h  ·  NO PSRAM", f8, FAINT)
    tr(d, W - 60, 700, "V1.0  /  2026-11-08", f8, FAINT)

    img.save(OUT)
    print("wrote", OUT)


if __name__ == "__main__":
    main()
