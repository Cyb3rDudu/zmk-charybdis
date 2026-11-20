# SWD-Rescue für den Holyiot-19077-Dongle

Der Dongle hat **keinen Button** und sein Bootloader ignoriert GPREGRET.
Wenn die App nicht bootet (z. B. nach einem blinden Flash), ist SWD der
einzige Weg zurück. Diese Toolchain hat den Dongle am 2026-11-20 gerettet.

## Benötigt
- Ein nice!nano-kompatibles Board (PandaKB Promicro NRF52840 o. ä.)
- 3 Drähte: nano D1→DD, nano D0→CLK, nano GND→Dongle-GND
- Optional VCC→VCC (nur wenn der Dongle NICHT am eigenen USB hängt —
  nie beide Stromquellen gleichzeitig!)

## Firmware bauen (im ZMK-Container)
```
west build -p -b nrf52840dongle/nrf52840/zmk -- \
  -DZMK_CONFIG=$PWD/../config -DSHIELD=charybdis_dongle \
  -DKEYMAP_FILE=overlays/charybdis.keymap
# → build/zephyr/zmk.bin + zmk.hex nach rescue/ kopieren
```
WICHTIG: das Shield-Overlay erzwingt reg0=disabled + reg1=LDO —
die Holyiot-Platine hat KEINE DCDC-Spulen bestückt, DCDC tötet die
App beim ersten Takt (siehe CLAUDE.md).

## Sketch bauen (arduino-cli, adafruit:nrf52:feather52840)
```
python3 tools/swd-rescue/generate_header.py rescue/dongle_fixed.h
# RescueDongle.ino + dongle_fixed.h in einen Sketch-Ordner,
# Adafruit_DAP-Library (mit NRF_NVMC-Typ-Guard gepatcht) einbinden,
# kompilieren, UF2 mit Family 0x239a00b3 konvertieren:
uf2conv.py -c -f 0x239a00b3 -o Flasher.uf2 build/sketch.hex
```

## Ablauf
1. Flasher-UF2 auf den nano (RST↔GND 2× → NICENANO-Laufwerk)
2. Drähte anlöten/andocken, Sketch hämmert automatisch auf SWD
3. `SUCCESS-LDO-FW-FLASHED!` im Serial (115200)
4. Drähte weg, Dongle an USB
5. Falls `1915:521f` (Open DFU Bootloader) erscheint: einmal sauber per
   nrfutil durch den Bootloader flashen (setzt die Validation-Settings):
   ```
   nrfutil pkg generate --hw-version 52 --sd-req 0x00 \
     --application rescue/dongle_fixed.hex --application-version 1 dfu.zip
   nrfutil dfu usb-serial -pkg dfu.zip -p /dev/ttyACM0
   ```
6. Bootloader-DFU zeigt sich teils erst nach PC-Neustart (Power-Cycle
   des Ports) — warmes Umstecken reicht nicht immer.

## Erkenntnisse (Blut geleckt)
- Bootloader sitzt bei 0xE0000, App bei 0x1000 (SlimeVR-Konvention)
- DFU-Eintritt des Bootloaders: P0.19=LOW beim Boot (Button-Emulation),
  NICHT GPREGRET (wird nie gelesen/gecleart)
- nrf-DFU-Flash hinterlässt Settings, die zur App passen MÜSSEN, sonst
  bootet der Bootloader immer in den DFU-Modus (nicht schlimm, nur laut)
- Der Bootloader läuft im LDO-Default → deshalb überlebt er immer,
  während DCDC-Apps sofort sterben


## Dongle-Kaufkriterien (blutige Lektion vom 2026-11-20)

Ein ZMK-Dongle mit Nordic Open DFU Bootloader BRAUCHT einen
physischen DFU-Weg. Vor dem Kauf prüfen:

| Kriterium | OK | Problemdongles |
|---|---|---|
| **Physischer Button** (wie PCA10059 / Holyiot 22046) | ✅ | — |
| **Magnet/Hall-Sensor** (Holyiot 21017) | ✅ | — |
| Kein Button, nur 4 SWD-Pads (Holyiot 19077) | ⛔ | App-Crash = eingesperrt |

Zusaetzlich: Holyiot bestueckt KEINE DCDC-Spulen (siehe LDO-Fix im
Shield-Overlay). Fuer einen Ersatz-Dongle: Firmware aus diesem Repo
auf einen 22046 flashen (Board nrf52840dongle, LDO-Overlay aktiv,
&P0.19-DFU-Verhalten enthalten) → laeuft first try, DFU-Taste
funktioniert ueber den echten Button-Mechanismus.
