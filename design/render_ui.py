# -*- coding: utf-8 -*-
"""Cardputer-Adv MIDI Player — UI mockups (Gruvbox Dark).

Drawn on a real 240x135 pixel canvas with 1 px strokes and the 8 px
Silkscreen font, then upscaled with nearest-neighbour.
Run:  python design/render_ui.py
"""

import os

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "out")
FONTS = os.path.join(HERE, "fonts")

W, H = 240, 135
S = 4

# ---------------------------------------------------------------- Gruvbox Dark
BG      = (29, 32, 33)     # dark0_hard  #1D2021
BAR     = (40, 40, 40)     # dark0       #282828
PANEL   = (40, 40, 40)
PADBG   = (50, 48, 47)     # dark0_soft  #32302F
EDGE    = (80, 73, 69)     # dark2       #504945
LINE    = (60, 56, 54)     # dark1       #3C3836
TXT     = (235, 219, 178)  # light1      #EBDBB2
LT2     = (213, 196, 161)  # light2      #D5C4A1
DIM     = (168, 153, 132)  # light4      #A89984
FAINT   = (124, 111, 100)  # dark4       #7C6F64
ACC     = (254, 128, 25)   # bright orange #FE8019
ACC_DIM = (74, 42, 23)     # own: orange at ~15 % on #1D2021
AMB     = (250, 189, 47)   # bright yellow #FABD2F
AMB_DIM = (58, 52, 23)     # own
AQUA    = (142, 192, 124)  # bright aqua   #8EC07C
BLUE    = (131, 165, 152)  # bright blue   #83A598
PURPLE  = (211, 134, 155)  # bright purple #D3869B
RED     = (251, 73, 52)    # bright red    #FB4934
GREEN   = (184, 187, 38)   # bright green  #B8BB26

f8   = ImageFont.truetype(os.path.join(FONTS, "Silkscreen-Regular.ttf"), 8)
f8b  = ImageFont.truetype(os.path.join(FONTS, "Silkscreen-Bold.ttf"), 8)
f16  = ImageFont.truetype(os.path.join(FONTS, "Silkscreen-Regular.ttf"), 16)
f16b = ImageFont.truetype(os.path.join(FONTS, "Silkscreen-Bold.ttf"), 16)
f32b = ImageFont.truetype(os.path.join(FONTS, "Silkscreen-Bold.ttf"), 32)


# ------------------------------------------------------------------ primitives
def txt(d, x, y, s, font=f8, fill=TXT):
    """Draw text with the top of the capitals at y."""
    off = {8: -4, 16: -7, 32: -13}.get(font.size, -4)
    d.text((x, y + off), s, font=font, fill=fill)


def tw(s, font=f8):
    return int(font.getlength(s))


def tr(d, right, y, s, font=f8, fill=TXT):
    txt(d, right - tw(s, font), y, s, font, fill)


def center(d, cx, y, s, font=f8, fill=TXT):
    txt(d, cx - tw(s, font) // 2, y, s, font, fill)


def rect(d, x, y, w, h, fill=None, outline=None):
    d.rectangle([x, y, x + w - 1, y + h - 1], fill=fill, outline=outline)


def hline(d, x, y, w, fill=LINE):
    d.line([x, y, x + w - 1, y], fill=fill)


def vline(d, x, y, h, fill=LINE):
    d.line([x, y, x, y + h - 1], fill=fill)


def bar(d, x, y, w, h, ratio, color=ACC, track=LINE):
    rect(d, x, y, w, h, track)
    fw = int((w - 2) * max(0.0, min(1.0, ratio)))
    if fw > 0:
        rect(d, x + 1, y + 1, fw, h - 2, color)


def screen():
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)
    d.fontmode = "1"
    return img, d


# ---------------------------------------------------------------------- icons
def icon_play(d, x, y, col=BG):
    d.polygon([(x, y), (x, y + 8), (x + 7, y + 4)], fill=col)


def icon_pause(d, x, y, col=BG):
    rect(d, x, y, 2, 8, col)
    rect(d, x + 4, y, 2, 8, col)


def icon_prev(d, x, y, col=TXT):
    rect(d, x, y, 2, 8, col)
    d.polygon([(x + 8, y), (x + 2, y + 4), (x + 8, y + 8)], fill=col)


def icon_next(d, x, y, col=TXT):
    d.polygon([(x, y), (x + 6, y + 4), (x, y + 8)], fill=col)
    rect(d, x + 6, y, 2, 8, col)


def icon_replay(d, x, y, col=BG):
    # 9x9 circular arrow
    d.arc([x, y, x + 8, y + 8], 40, 320, fill=col)
    d.polygon([(x + 6, y - 2), (x + 9, y + 1), (x + 5, y + 2)], fill=col)


def icon_vol(d, x, y, col=DIM, level=2):
    d.polygon([(x, y + 2), (x + 3, y + 2), (x + 6, y), (x + 6, y + 8), (x + 3, y + 6), (x, y + 6)],
              fill=col)
    if level >= 1:
        d.line([x + 8, y + 2, x + 8, y + 6], fill=col)
    if level >= 2:
        d.line([x + 10, y, x + 10, y + 8], fill=col)


def icon_folder(d, x, y, col=AMB):
    rect(d, x, y + 1, 8, 6, outline=col)
    rect(d, x, y, 4, 2, col)


def icon_note(d, x, y, col=BLUE):
    rect(d, x + 4, y, 1, 7, col)
    d.polygon([(x + 5, y), (x + 5, y + 3), (x + 2, y + 3), (x + 2, y + 1)], fill=col)
    d.ellipse([x + 1, y + 5, x + 4, y + 8], fill=col)


def icon_eq(d, x, y, col=ACC):
    rect(d, x, y + 4, 2, 4, col)
    rect(d, x + 3, y + 1, 2, 7, col)
    rect(d, x + 6, y + 3, 2, 5, col)


def icon_battery(d, x, y, pct=82):
    rect(d, x, y, 12, 6, None, DIM)
    rect(d, x + 12, y + 2, 2, 2, DIM)
    n = 8 if pct > 70 else 5 if pct > 35 else 2
    rect(d, x + 1, y + 1, n, 4, GREEN if pct > 35 else RED)


def icon_card(d, x, y, col=RED):
    # 24 x 30 SD card, cut corner, label area
    d.polygon([(x, y), (x + 17, y), (x + 23, y + 6), (x + 23, y + 29), (x, y + 29)], outline=col)
    d.line([x + 17, y + 1, x + 17, y + 6], fill=col)
    d.line([x + 17, y + 6, x + 22, y + 6], fill=col)
    rect(d, x + 4, y + 9, 16, 12, None, col)
    txt(d, x + 6, y + 12, "SD", f8b, col)


# --------------------------------------------------------------- piano widget
BLACKS = {1, 3, 6, 8, 10}  # semitones above C that are black


def piano(d, x, y, nkeys, keyw, h, base=60, active=(), felt=False, labels=()):
    """nkeys consecutive white keys starting at MIDI note `base` (a C)."""
    semis = []
    n = base
    while len(semis) < nkeys:
        if (n % 12) not in BLACKS:
            semis.append(n)
        n += 1
    for i, s in enumerate(semis):
        xk = x + i * keyw
        on = s in active
        rect(d, xk, y, keyw, h, ACC if on else LT2)
        if i < nkeys - 1:
            vline(d, xk + keyw - 1, y, h, BG)
    # black keys sit on the boundary after C,D,F,G,A (mod 12)
    for i, s in enumerate(semis[:-1]):
        if ((s + 1) % 12) in BLACKS:
            bw = 7 if keyw >= 14 else (5 if keyw >= 8 else 3)
            bx = x + (i + 1) * keyw - bw // 2
            on = (s + 1) in active
            rect(d, bx, y, bw, (h * 10) // 18, ACC if on else BG)
    if felt:
        rect(d, x, y + h, nkeys * keyw, 2, ACC)
    for i, lab, s in labels:
        on = s in active
        txt(d, x + i * keyw + 2, y + h - 6, lab, f8, BG if on else FAINT)


def logo(d, x, y):
    # eighth note, 9 x 9
    rect(d, x + 4, y, 1, 7, ACC)
    rect(d, x + 4, y, 3, 1, ACC)
    rect(d, x + 6, y + 1, 1, 2, ACC)
    d.ellipse([x + 1, y + 5, x + 4, y + 8], fill=ACC)


# ------------------------------------------------------------------ chrome
def topbar(d, section, sd="ok", pct=82):
    rect(d, 0, 0, W, 13, BAR)
    hline(d, 0, 12, W, LINE)
    logo(d, 5, 2)
    txt(d, 17, 4, "MIDI", f8b, ACC)
    vline(d, 46, 3, 7, LINE)
    txt(d, 52, 4, section, f8, FAINT)
    col = GREEN if sd == "ok" else AMB if sd == "busy" else RED
    rect(d, 185, 5, 5, 5, col)
    txt(d, 193, 4, "SD", f8b, col)
    icon_battery(d, 220, 4, pct)


def hints(d, items, gap=8):
    widths = [tw(k, f8b) + 6 + tw(a) for k, a in items]
    total = sum(widths) + gap * (len(items) - 1)
    x = (W - total) // 2
    for (k, a), w in zip(items, widths):
        txt(d, x, 127, k, f8b, TXT)
        txt(d, x + tw(k, f8b) + 6, 127, a, f8, DIM)
        x += w + gap


def bottombar(d, items):
    rect(d, 0, 124, W, 11, BAR)
    hline(d, 0, 124, W, LINE)
    hints(d, items)


def badge(d, right, y, label, fg=ACC, bgc=ACC_DIM):
    w = tw(label, f8b) + 8
    x = right - w
    rect(d, x, y, w, 11, bgc)
    rect(d, x, y, 2, 11, fg)
    txt(d, x + 5, y + 3, label, f8b, fg)


# --------------------------------------------------------------- 01 splash
def draw_splash():
    img, d = screen()
    txt(d, 5, 6, "M5STACK  ·  CARDPUTER-ADV", f8, FAINT)
    tr(d, 235, 6, "V0.1", f8, FAINT)

    center(d, 120, 24, "MIDI", f32b, TXT)
    center(d, 120, 51, "PIANO PLAYER", f8b, ACC)
    hline(d, 96, 64, 48, LINE)

    center(d, 120, 76, "SCANNING SD CARD", f8, DIM)
    bar(d, 70, 88, 100, 4, 0.45, ACC)
    center(d, 120, 98, "12 FILES FOUND", f8, FAINT)

    rect(d, 0, 109, W, 2, ACC)
    piano(d, 1, 111, 14, 17, 23, base=48, active={60, 64, 67})
    return img


# --------------------------------------------------------------- 02 library
FILES = [
    ("folder", "CHOPIN",   "", "DIR"),
    ("folder", "DEBUSSY",  "", "DIR"),
    ("mid",    "CLAIR_DE_LUNE.MID", "3:47", "18K"),
    ("mid",    "FLOWER_DANCE.MID",  "2:31", "14K"),
    ("mid",    "GYMNOPEDIE_1.MID",  "3:12", "11K"),
    ("mid",    "RIVER_FLOWS_IN_YOU.MID", "4:06", "21K"),
]


def draw_library(sel=2, playing=True):
    img, d = screen()
    topbar(d, "LIBRARY")

    rect(d, 0, 13, W, 12, PANEL)
    hline(d, 0, 24, W, LINE)
    icon_folder(d, 5, 16, AMB)
    txt(d, 17, 16, "SD:/MIDI", f8b, BLUE)
    tr(d, 235, 16, "12 FILES", f8, FAINT)

    y0, rowh = 26, 16
    for i, (kind, name, dur, size) in enumerate(FILES):
        y = y0 + i * rowh
        selected = i == sel
        if not selected:
            hline(d, 0, y + rowh - 1, 236, LINE)
        if selected:
            rect(d, 0, y, 236, rowh, ACC)
        fg = BG if selected else TXT
        fg2 = BG if selected else DIM
        fg3 = BG if selected else FAINT

        if kind == "folder":
            icon_folder(d, 6, y + 4, BG if selected else AMB)
        elif selected and playing:
            icon_eq(d, 6, y + 4, BG)
        else:
            icon_note(d, 6, y + 4, BG if selected else BLUE)

        txt(d, 18, y + 5, name, f8b if selected else f8, fg)

        if kind == "folder":
            tr(d, 232, y + 5, "DIR", f8, fg3)
        else:
            tr(d, 206, y + 5, dur, f8, fg2)
            tr(d, 232, y + 5, size, f8, fg3)

    rect(d, 237, y0, 2, 96, PADBG)
    rect(d, 237, y0 + 4, 2, 46, FAINT)
    bottombar(d, [("ENTER", "PLAY"), ("SPC", "PREVIEW"), ("BKSP", "UP")])
    return img


# ---------------------------------------------------------------- 03 player
def draw_player(paused=False, ended=False):
    img, d = screen()
    topbar(d, "PLAYER")

    rect(d, 4, 15, 36, 36, PADBG, EDGE)
    piano(d, 8, 27, 7, 4, 13, active=(), labels=())
    rect(d, 8, 41, 28, 1, ACC)

    txt(d, 46, 16, "CLAIR DE LUNE", f8b, TXT)
    txt(d, 46, 26, "DEBUSSY", f8, DIM)
    txt(d, 46 + tw("DEBUSSY") + 6, 26, "·", f8, FAINT)
    txt(d, 46 + tw("DEBUSSY") + 13, 26, "9 TRACKS", f8, BLUE)
    x3 = 46
    for i, (s, col) in enumerate((("SMF 1", FAINT), ("·", FAINT), ("66.0 BPM", DIM),
                                  ("·", FAINT), ("DB MAJ", DIM))):
        txt(d, x3, 36, s, f8, col)
        x3 += tw(s) + 6

    if ended:
        badge(d, 235, 14, "ENDED", FAINT, PADBG)
    elif paused:
        badge(d, 235, 14, "PAUSED", AMB, AMB_DIM)
    else:
        badge(d, 235, 14, "PLAYING", ACC, ACC_DIM)

    hline(d, 0, 51, W, LINE)

    txt(d, 5, 53, "3:47" if ended else "1:12", f16b, DIM if ended else TXT)
    txt(d, 55, 58, "/", f8, FAINT)
    txt(d, 63, 58, "3:47", f8, DIM)
    tr(d, 235, 58, "BAR 96/96" if ended else "BAR 31/96", f8, FAINT)

    ratio = 1.0 if ended else 0.318
    bar(d, 5, 71, 230, 5, ratio, FAINT if ended else (AMB if paused else ACC))
    if not ended:
        hx = 5 + int(230 * ratio)
        rect(d, hx - 1, 69, 3, 9, AMB if paused else TXT)

    # live strip
    txt(d, 5, 79, "LIVE", f8, FAINT)
    if ended:
        txt(d, 35, 79, "IDLE", f8, FAINT)
    elif paused:
        rect(d, 36, 80, 2, 5, AMB)
        rect(d, 40, 80, 2, 5, AMB)
        txt(d, 47, 79, "C#4 F4 G#4", f8, AMB)
    else:
        txt(d, 35, 79, "C#4 F4 G#4", f8, TXT)
    rect(d, 144, 77, 28, 11, None, AQUA if not (paused or ended) else EDGE)
    txt(d, 149, 80, "PED", f8, AQUA if not (paused or ended) else FAINT)
    txt(d, 181, 79, "VOX", f8, FAINT)
    txt(d, 205, 79, "00/24" if (paused or ended) else "03/24", f8, TXT)

    active = () if (paused or ended) else (61, 65, 68)
    piano(d, 8, 89, 14, 16, 18, base=48, active=active,
          labels=((0, "C3", 48), (7, "C4", 60)))

    # transport
    yb = 109
    rect(d, 5, yb, 18, 14, None, EDGE)
    icon_prev(d, 9, yb + 3, TXT)
    rect(d, 27, yb, 34, 14, ACC)
    if paused or ended:
        icon_play(d, 41, yb + 3, BG)
    else:
        icon_pause(d, 41, yb + 3, BG)
    rect(d, 65, yb, 18, 14, None, EDGE)
    icon_next(d, 70, yb + 3, TXT)

    rect(d, 87, yb, 42, 14, None, EDGE)
    rect(d, 93, yb + 5, 5, 5, None, FAINT)
    txt(d, 102, yb + 4, "LOOP", f8, FAINT)

    txt(d, 154, yb + 4, "VOL", f8, FAINT)
    icon_vol(d, 175, yb + 3, DIM, 2)
    bar(d, 188, yb + 5, 32, 4, 0.78, AQUA)
    txt(d, 224, yb + 4, "78", f8b, TXT)

    if ended:
        bottombar(d, [("SPC", "REPLAY"), ("BKSP", "LIST")])
    elif paused:
        bottombar(d, [("SPC", "RESUME"), ("ARROWS", "SEEK"), ("BKSP", "LIST")])
    else:
        bottombar(d, [("SPC", "PLAY/PAUSE"), ("ARROWS", "SEEK"), ("BKSP", "LIST")])
    return img


# ------------------------------------------------------------ 06 no files
def draw_no_files():
    img, d = screen()
    topbar(d, "LIBRARY")
    rect(d, 0, 13, W, 12, PANEL)
    hline(d, 0, 24, W, LINE)
    icon_folder(d, 5, 16, AMB)
    txt(d, 17, 16, "SD:/MIDI", f8b, BLUE)
    tr(d, 235, 16, "0 FILES", f8, FAINT)

    # 44 x 32 folder
    rect(d, 98, 30, 18, 7, AMB)
    rect(d, 98, 36, 44, 32, None, AMB)
    hline(d, 99, 42, 42, AMB)

    center(d, 120, 78, "NO MIDI FILES", f16b, TXT)
    center(d, 120, 96, "PUT .MID FILES IN SD:/MIDI", f8, DIM)
    rect(d, 72, 107, 96, 13, None, ACC)
    center(d, 120, 111, "RESCAN  ENTER", f8b, ACC)
    bottombar(d, [("ENTER", "RESCAN"), ("BKSP", "ROOT")])
    return img


# -------------------------------------------------------------- 07 no card
def draw_no_sd():
    img, d = screen()
    topbar(d, "LIBRARY", sd="error")
    rect(d, 0, 13, W, 12, PANEL)
    hline(d, 0, 24, W, LINE)
    icon_folder(d, 5, 16, FAINT)
    txt(d, 17, 16, "SD:/", f8b, FAINT)
    tr(d, 235, 16, "-- FILES", f8, FAINT)

    icon_card(d, 104, 32, RED)
    center(d, 120, 72, "NO SD CARD", f16b, RED)
    center(d, 120, 91, "INSERT CARD, THEN PRESS ENTER", f8, DIM)
    rect(d, 80, 105, 80, 13, None, RED)
    center(d, 120, 109, "RETRY  ENTER", f8b, RED)
    bottombar(d, [("ENTER", "RETRY"), ("BKSP", "EXIT")])
    return img


# --------------------------------------------------------------- overview
def draw_controls_panel(d, x, y, w, h):
    rect(d, x, y, w, h, BG, EDGE)
    txt(d, x + 20, y + 18, "CONTROLS", f16b, TXT)
    hline(d, x + 20, y + 40, w - 40, ACC)
    rows = [
        ("SPACE", "PLAY / PAUSE"),
        ("ARROWS", "SEEK -5 / +5 S"),
        ("ENTER", "OPEN / PLAY"),
        ("BKSP", "BACK / LIST"),
    ]
    for i, (key, action) in enumerate(rows):
        yy = y + 58 + i * 38
        rect(d, x + 20, yy + 1, 8, 8, ACC)
        txt(d, x + 38, yy, key, f16b, TXT)
        txt(d, x + 38 + tw(key, f16b) + 14, yy, action, f16, DIM)
    hline(d, x + 20, y + h - 42, w - 40, LINE)
    txt(d, x + 20, y + h - 32, "ORANGE = PLAY  ·  YELLOW = PAUSE  ·  RED = ERROR", f8, FAINT)


def draw_overview(images):
    cols, rows = 2, 4
    cw, ch = 480, 270
    pad, cap = 24, 26
    margin, header = 28, 72
    sw = margin * 2 + cols * cw + (cols - 1) * pad
    sh = header + rows * (ch + cap) + (rows - 1) * pad + margin
    img = Image.new("RGB", (sw, sh), BG)
    d = ImageDraw.Draw(img)
    d.fontmode = "1"

    txt(d, margin, 20, "MIDI PIANO", f32b, TXT)
    txt(d, margin, 49, "CARDPUTER-ADV  ·  240×135  ·  GRUVBOX DARK  ·  SILKSCREEN 8", f8, FAINT)
    hline(d, margin, 62, sw - margin * 2, ACC)

    keys = ["splash", "library", "player", "paused", "ended", "no_files", "no_sd"]
    caps = ["01  SCANNING", "02  LIBRARY", "03  PLAYING", "04  PAUSED", "05  ENDED",
            "06  NO FILES", "07  NO SD CARD"]
    for i, (key, cap_) in enumerate(zip(keys, caps)):
        c, r = i % cols, i // cols
        x = margin + c * (cw + pad)
        y = header + r * (ch + cap + pad)
        img.paste(images[key].resize((cw, ch), Image.NEAREST), (x, y))
        d.rectangle([x - 1, y - 1, x + cw, y + ch], outline=EDGE)
        txt(d, x, y + ch + 7, cap_, f16, ACC if i in (1, 2) else DIM)

    # 8th cell: controls / semantics card
    c, r = 7 % cols, 7 // cols
    x = margin + c * (cw + pad)
    y = header + r * (ch + cap + pad)
    draw_controls_panel(d, x, y, cw, ch)
    d.rectangle([x - 1, y - 1, x + cw, y + ch], outline=EDGE)
    return img


# --------------------------------------------------------------- palette
def draw_palette():
    swatches = [
        ("BG", BG), ("BAR", BAR), ("PAD", PADBG), ("EDGE", EDGE), ("LINE", LINE),
        ("TXT", TXT), ("DIM", DIM), ("FAINT", FAINT), ("ACC", ACC), ("AMB", AMB),
        ("AQUA", AQUA), ("BLUE", BLUE), ("PURPLE", PURPLE), ("RED", RED), ("GREEN", GREEN),
    ]
    cw, gap, top, cols = 96, 10, 66, 8
    rows = 2
    sw = 24 * 2 + cols * cw + (cols - 1) * gap
    sh = top + rows * 96 + 18
    img = Image.new("RGB", (sw, sh), BG)
    d = ImageDraw.Draw(img)
    d.fontmode = "1"
    txt(d, 24, 18, "GRUVBOX DARK", f16b, TXT)
    txt(d, 24, 39, "PALETTE  /  SEMANTICS", f8, FAINT)
    for i, (name, col) in enumerate(swatches):
        c, r = i % cols, i // cols
        x = 24 + c * (cw + gap)
        y = top + r * 96
        rect(d, x, y, cw, 48, col)
        d.rectangle([x, y, x + cw - 1, y + 47], outline=EDGE)
        txt(d, x, y + 56, name, f8b, TXT)
        txt(d, x, y + 68, "#%02X%02X%02X" % col, f8, FAINT)
    return img


def main():
    os.makedirs(OUT, exist_ok=True)
    imgs = {
        "splash": draw_splash(),
        "library": draw_library(),
        "player": draw_player(),
        "paused": draw_player(paused=True),
        "ended": draw_player(ended=True),
        "no_files": draw_no_files(),
        "no_sd": draw_no_sd(),
    }
    names = {
        "splash": "01-splash",
        "library": "02-library",
        "player": "03-player",
        "paused": "04-paused",
        "ended": "05-ended",
        "no_files": "06-no-files",
        "no_sd": "07-no-sd",
    }
    for k, im in imgs.items():
        im.resize((W * S, H * S), Image.NEAREST).save(os.path.join(OUT, names[k] + ".png"))

    draw_overview(imgs).save(os.path.join(OUT, "00-overview.png"))
    draw_palette().save(os.path.join(OUT, "palette.png"))
    print("wrote", OUT)


if __name__ == "__main__":
    main()
