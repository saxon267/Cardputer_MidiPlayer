#!/usr/bin/env python3
"""Generate extra parser edge-case MIDI files."""
import struct, os

PPQ = 96

def vlq(n):
    out = [n & 0x7F]
    n >>= 7
    while n:
        out.append((n & 0x7F) | 0x80)
        n >>= 7
    return bytes(reversed(out))

def track(events):
    data = b"".join(events)
    return b"MTrk" + struct.pack(">I", len(data)) + data

def meta(delta, typ, payload=b""):
    return vlq(delta) + bytes([0xFF, typ]) + vlq(len(payload)) + payload

def ch(delta, *data):
    return vlq(delta) + bytes(data)

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")

# Main test: Format 1, C major scale + chord, 120 BPM, 4/4, 5.0 s.
t0 = b""
t0 += meta(0, 0x03, b"TEST SCALE")
t0 += meta(0, 0x51, b"\x07\xA1\x20")
t0 += meta(0, 0x58, b"\x04\x02\x18\x08")
t0 += meta(0, 0x59, b"\x00\x00")
t0 += meta(0, 0x2F)

t1 = b""
t1 += meta(0, 0x03, b"PIANO")
notes = [60, 62, 64, 65, 67, 69, 71, 72]
for n in notes:
    t1 += ch(0, 0x90, n, 100)
    t1 += ch(480, 0x80, n, 0)
t1 += ch(0, 0xB0, 64, 127)
for n in (60, 64, 67):
    t1 += ch(0, 0x90, n, 90)
t1 += ch(960, 0x80, 60, 0) + ch(0, 0x80, 64, 0) + ch(0, 0x80, 67, 0)
t1 += ch(0, 0xB0, 64, 0)
t1 += meta(0, 0x2F)

data = b"MThd" + struct.pack(">IHHH", 6, 1, 2, 480) + track([t0]) + track([t1])
open(os.path.join(root, "test.mid"), "wb").write(data)
print("test.mid", len(data), "bytes")

# Running status: one 0x90, then bare data bytes; one 0x80, then bare data.
t = b""
t += meta(0, 0x51, b"\x07\xA1\x20")  # 500000us
t += ch(0, 0x90, 60, 100)
t += ch(96, 62, 100)                  # running status
t += ch(96, 64, 100)                  # running status
t += ch(96, 0x80, 60, 0)
t += ch(0, 62, 0)                     # running status
t += ch(0, 64, 0)                     # running status
t += meta(0, 0x2F)
data = b"MThd" + struct.pack(">IHHH", 6, 0, 1, PPQ) + track([t])
open(os.path.join(root, "test_running.mid"), "wb").write(data)
print("test_running.mid", len(data), "bytes")

# Tempo change at tick 480.
t = b""
t += meta(0, 0x51, b"\x07\xA1\x20")  # 500000us
t += ch(0, 0x90, 60, 100)
t += meta(480, 0x51, b"\x03\xD0\x90")  # 250000us
t += ch(480, 0x80, 60, 0)
t += meta(0, 0x2F)
data = b"MThd" + struct.pack(">IHHH", 6, 0, 1, 480) + track([t])
open(os.path.join(root, "test_tempo.mid"), "wb").write(data)
print("test_tempo.mid", len(data), "bytes")
