#!/usr/bin/env python3
"""Erzeugt dongle_fixed.h (eingebettete Firmware) aus rescue/dongle_fixed.bin
fuer den RescueDongle-Arduino-Sketch (PandaKB/nice!nano als SWD-Flasher)."""
import sys

src = sys.argv[1] if len(sys.argv) > 1 else 'rescue/dongle_fixed.bin'
dst = sys.argv[2] if len(sys.argv) > 2 else 'dongle_fixed.h'
data = open(src, 'rb').read()
with open(dst, 'w') as f:
    f.write(f"const uint32_t firmware_len = {len(data)};\nconst uint8_t firmware[] = {{\n")
    for i in range(0, len(data), 16):
        f.write("  " + ", ".join(f"0x{b:02X}" for b in data[i:i+16]) + ",\n")
    f.write("};\n")
print(f"{dst}: {len(data)} bytes eingebettet")
