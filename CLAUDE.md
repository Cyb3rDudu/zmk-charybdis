# CLAUDE.md — ZMK Charybdis Firmware

## Project Overview

ZMK firmware for the Wireless Charybdis 3x6 ergonomic split keyboard with PMW3610 trackball sensor. Supports Bluetooth/USB and 2.4GHz dongle configurations, targeting the Nice!Nano v2 board.

## Repository Structure

```
boards/shields/
  charybdis_bt/         # Bluetooth/USB shield (device tree overlays, Kconfig)
  charybdis_dongle/     # 2.4GHz dongle shield
config/
  keymap/               # Keymap files (.keymap) — add/remove to control what gets built
    qwerty.keymap       # Default keymap
    behaviors.dtsi      # Custom hold-tap, mod-morph, tap-dance behaviors
    combos.dtsi         # Combo key definitions
    macros.dtsi         # Macro definitions (TMUX shortcuts, etc.)
  charybdis.conf        # Global ZMK configuration
  charybdis_left.conf   # Left half config
  charybdis_right.conf  # Right half + trackball config
  charybdis_pointer.dtsi  # Trackball input processors (speed, scroll, precision)
  charybdis_pmw3610.dtsi  # PMW3610 sensor hardware config
  charybdis-layouts.dtsi  # Physical key layout (ZMK Studio compatible)
  west.yml              # West manifest (ZMK + PMW3610 driver dependencies)
local-build/            # Docker-based local build system
  build_setup.sh        # Main build script
  docker-compose.yml    # Container config
scripts/                # Build automation (matrix generation, keymap conversion)
firmwares/              # Build output directory (.uf2 files)
build.yaml              # GitHub Actions build matrix config
```

## Build Commands

**Local build (Docker — recommended for development):**
```sh
cd local-build && docker-compose run --rm builder
```

**Debug shell inside container:**
```sh
cd local-build && docker-compose run --rm --entrypoint bash builder
# Then: bash ./local-build/build_setup.sh
```

**GitHub Actions:** Push to GitHub; workflows build automatically.

Firmware output lands in `firmwares/` organized by format and keymap.

## Key Technical Details

- **Languages:** Zephyr Device Tree (.dtsi/.overlay), ZMK Keymap DSL (.keymap), Kconfig, YAML, Python, Bash
- **Dependencies:** ZMK firmware (main), zmk-pmw3610-driver (280Zo fork), Zephyr RTOS
- **West manifest:** `config/west.yml` — manages ZMK and driver dependencies
- **Board:** nice_nano_v2 (halves), nrf52840dongle/nrf52840/zmk (dongle shield)
- **Formats:** bt (Bluetooth), dongle (2.4GHz), reset (settings clear)

## Dongle Setup (Holyiot 19077, nRF52840)

The `dongle` format builds three firmwares: `charybdis_left` and
`charybdis_right` as pure split peripherals (Central role off) and
`charybdis_dongle` as the USB-attached central running on the
`nrf52840dongle` board. ZMK Studio runs on the dongle
(`studio-rpc-usb-uart` snippet).

### Flashing order

1. Flash `settings_reset` (UF2) on **both** halves, wait for them to boot once
2. Flash the dongle-*peripheral* UF2s on the halves
3. Flash the dongle last: it ships with the Nordic **Open DFU Bootloader**;
   use `charybdis_dongle.hex` with nRF Connect for Desktop → Programmer
   (or `nrfutil dfu usb-serial`). The dongle has **no reset button**.

### ⚠️ CRITICAL: DCDC kills the app on Holyiot hardware

The Holyiot 19077 (and 21017) PCBs **do not populate the DCDC inductors**.
The stock Zephyr `nrf52840dongle` board enables `NRF5X_REG_MODE_DCDC` for
reg1 — on the Holyiot clone this collapses VDD and the app dies within its
first cycles (no USB, no BLE, nothing). The bootloader survives because it
runs with LDO defaults, which is why DFU always worked while apps never
booted.

`boards/shields/charybdis_dongle/charybdis_dongle.overlay` therefore forces:
```dts
&reg0 { status = "disabled"; };
&reg1 { regulator-initial-mode = <NRF5X_REG_MODE_LDO>; };
```
(mirrors SlimeVR's `holyiot_21017` board definition — the only community
proven recipe for Holyiot dongles). **Never build a dongle image without
this overlay.** USB powered, so LDO quiescent draw is irrelevant.

### FINAL STATE (2026-11-20, evening) — dongle is FROZEN

The last rescue attempt tore off the **SWD CLK pad**. With no SWD and no
reachable DFU entry (app valid, GPREGRET ignored, P0.19 not exposed),
the dongle firmware is **permanently frozen** at the v11 build:
reg0-off + reg1-LDO, USB HID central, no Studio, `&nordic_dfu` = GPREGRET
(no-op on this bootloader). The `b8a5897` commits (P0.19 behavior +
Studio on dongle) are in the repo but were never flashed — keep them for
a future dongle with intact pads.

**The dongle works fine as-is.** Do not power-cycle it into oblivion:
it boots reliably (LDO fix), halves connect, typing works.

**Keymap changes from now on → bt mode:**
1. Push keymap changes to this repo (workflow builds everything)
2. Flash BOTH halves with the `qwerty-bt` UF2 artifacts (2× reset → NICENANO)
   — after a `settings_reset` round on both halves (topology change!)
3. Right half = central → ZMK Studio + keymap live there, as before
4. The dongle-mode stays on the frozen keymap; going back means
   re-flashing the halves with `qwerty-dongle` artifacts (works —
   the dongle itself boots whatever halves connect to it)

### Bootloader re-entry — what actually works

- The Holyiot Open DFU Bootloader **ignores GPREGRET** (never reads or
  clears it). `&nordic_dfu` writing `0xB1` does nothing on the dongle.
- Real DFU entry of this bootloader family: **P0.19 held LOW at boot**
  (the pca10059 button pin; SlimeVR drives GPIO19 low + reboots).
  Not wired on the 19077 — theoretical only.
- On the nice!nano/PandaKB halves `&nordic_dfu` writes `0x57`
  (DFU_MAGIC_UF2_RESET) and reliably drops the board into the
  NICENANO UF2 drive. **This path is proven and works.**
- If the dongle app is ever broken again: `tools/swd-rescue/` documents
  the full rescue (nice!nano as CMSIS-DAP-style SWD flasher, no debugger
  purchase needed). After an SWD flash, expect one `nrfutil` DFU round
  through the bootloader (settings re-validation) — the bootloader's DFU
  mode sometimes only enumerates after a full PC reboot (port power cycle).

### ZMK Studio on the dongle

The `studio-rpc-usb-uart` snippet build for the dongle was never validated
end-to-end; the running dongle firmware is built WITHOUT the snippet
(plain USB HID + split central). Studio stays on the bt builds / halves
as before. The workflow still adds the snippet for `charybdis_dongle` —
flip that off in `.github/workflows/build.yml` if it misbehaves.

### Implementation pointer

Zephyr module at `config/` (`config/zephyr/module.yml`,
`config/src/behaviors/behavior_nordic_dfu.c`,
`config/dts/bindings/zmk,behavior-nordic-dfu.yaml`).

## Layer Architecture

| # | Layer    | Purpose                                          |
|---|----------|--------------------------------------------------|
| 0 | BASE     | Standard typing with timeless home-row mods      |
| 1 | NUM      | Digits + F-keys (tap = number, shift = F-key)    |
| 2 | NAV      | Arrows, paging, TMUX, mouse pointer              |
| 3 | SYM      | Symbols and punctuation                          |
| 4 | GAME     | Pure keycodes, no mods                           |
| 5 | EXTRAS   | BT profiles, brightness, media, shortcuts        |
| 6 | MOUSE    | Full mouse-key layer                             |
| 7 | SLOW     | Low-speed pointer (precision)                    |
| 8 | SCROLL   | Vertical/horizontal scroll mode                  |

## Conventions

- Keymaps go in `config/keymap/<name>.keymap` — auto-discovered by build system
- Shields live in `boards/shields/charybdis_<format>/`
- Behaviors, combos, macros are separate `.dtsi` files included by keymaps
- Trackball tuning is in `config/charybdis_pointer.dtsi`
- Physical layout changes go in `config/charybdis-layouts.dtsi`

## Common Tasks

- **Add a new keymap:** Copy an existing `.keymap` from `config/keymap/` or (removed), modify, rebuild
- **Modify trackball behavior:** Edit `config/charybdis_pointer.dtsi` (speed, scroll scaling, axis inversion)
- **Change home-row mods:** Edit `config/keymap/behaviors.dtsi`
- **Add combos:** Edit `config/keymap/combos.dtsi`
- **Build only one format:** Remove the unwanted shield directory from `boards/shields/`
- **Enable USB logging:** Set variable at top of `local-build/build_setup.sh`, add Kconfig options to `config/charybdis_right.conf`
