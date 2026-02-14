[![.github/workflows/build.yml](https://github.com/280Zo/charybdis-wireless-mini-zmk-firmware/actions/workflows/build.yml/badge.svg)](https://github.com/280Zo/charybdis-wireless-mini-zmk-firmware/actions/workflows/build.yml)

## Intro

This repository offers pre-configured ZMK firmware designed for [Wireless Charybdis keyboards](https://github.com/280Zo/charybdis-wireless-mini-3x6-build-guide?tab=readme-ov-file). It supports both Bluetooth/USB and Dongle configurations and uses the latest input listener and processors to act as a bridge between the trackball and the rest of the system.

If you want to customize things the repo is set up to build through GitHub Actions (just clone and run it), or you can use the
containerized build script that will build all firmwares locally with a single command.


## Quick Start

### Flashing the Firmware

Download your choice of firmware from the Releases page. Choose a combination of format (Bluetooth/Dongle) and layout (QWERTY, etc.), then follow the steps below to flash it to your keyboard

1. Unzip the firmware zip
2. Plug the right half into the computer through USB
3. Double press the reset button
4. The keyboard will mount as a removable storage device
5. Copy the applicable uf2 file into the NICENANO storage device (e.g. charybdis_qwerty_dongle.uf2 -> dongle)
6. It will take a few seconds, then it will unmount and restart itself.
7. Repeat these steps for all devices.
8. You should now be able to use your keyboard

> [!NOTE]  
> If you are flashing the firmware for the first time, or if you're switching between the dongle and the Bluetooth/USB configuration, flash the reset firmware to all the devices first

### Overview & Usage

![keymap base](keymap-drawer/base/qwerty.svg)

To see all the layers check out the [full render](keymap-drawer/qwerty.svg).

#### Layers
| # | Layer      | Access                          | Purpose                                       |
| - | ---------- | ------------------------------- | --------------------------------------------- |
| 0 | **QWRT**   | Default                         | Standard QWERTY typing                        |
| 1 | **NUM**    | Hold right thumb (K40)          | Numbers, brackets, symbols, arithmetic         |
| 2 | **NAVI**   | —                               | Arrow keys, delete, clipboard paste            |
| 3 | **SCROLL** | Hold left thumb (K37)           | Arrow keys, delete, clipboard paste            |
| 4 | **SNIPE**  | Hold top-left key (K00)         | F-keys, Bluetooth, media, brightness           |

#### Modifiers
| Position    | Key                              |
| ----------- | -------------------------------- |
| Left outer  | `Shift` (row 1), `Ctrl` (row 2) |
| Right outer | `Shift` (row 1), `Ctrl` (row 2) |

#### Thumb Cluster
| Key  | Left Hand                                   |     | Key  | Right Hand          |
| ---- | ------------------------------------------- | --- | ---- | ------------------- |
| K36  | Left mouse click                            |     | K39  | Enter               |
| K37  | Hold: SCROLL layer                          |     | K40  | Hold: NUM layer     |
| K38  | Space                                       |     |      |                     |

#### Key Highlights
- **Layer-tap on Tab (K00):** Tap for Tab, hold for SNIPE layer (F-keys, BT, media, brightness)
- **SCROLL layer (K37):** Hold for arrow keys + delete + clipboard paste (`Gui+Shift+V`), right thumb becomes `Alt` and `Gui`
- **NUM layer (K40):** Numbers on home row, brackets and quotes on top row, `_ - = +` on bottom row, right-click on left thumb
- **Bluetooth profile switching:** Access via SNIPE layer — `BT_SEL 0-2` and `BT_CLR`
- **Media controls:** Access via SNIPE layer — prev/play/next, mute, volume, brightness
- **Caps Word:** `&caps_word` available (continues on `_` and `-`)
- **PMW3610 trackball sensor driver:** Provided by [280Zo](https://github.com/280Zo/zmk-pmw3610-driver) (fork of [badjeff](https://github.com/badjeff/zmk-pmw3610-driver))


## Customize Keymaps, Layers, & Trackball

This section will help you personalize your firmware. Everything—from keys and layers to advanced trackball behaviors—can easily be customized, even if you're new to ZMK.

### Building Only Specific Keymaps or Shields

By default both Bluetooth and Dongle formats will build firmware pairs for the QWERTY, Coleman DH, and Graphite keymaps. To save time and streamline your builds, you can build just a single keymap or shield that you're interested in:

**Single keymap:**
Any `.keymap` files in the `config/keymap/` directory will be automatically built. By default QWERTY and Colemak DH are included, but you can add or remove as many as you'd like as long as there is at least one .keymap file to process.
Some additional keymaps are available in the extra-keymaps directory.

**Single shield format (Dongle or Bluetooth):**
Delete the shield directory (charybdis_dongle or charybdis_bt) from the config/boards/shields/ folder to build only the format you need.


### Modify Key Mappings

**ZMK Studio**

[ZMK Studio](https://zmk.studio/) allows users to update functionality during runtime. It's currently only implemented on the Bluetooth builds.

To change the visual layout of the keys, the physical layout must be updated. This is the charybdis-layouts.dtsi file, which handles the actual physical positions of the keys. Though they may appear to be similar, this is different than the matrix transform file (charybdis.json) which handles the electrical matrix to keymap relationship.

To easily modify the physical layout, or convert a matrix transform file, [caksoylar](https://github.com/caksoylar/zmk-physical-layout-converter) has built the [ZMK physical layouts converter](https://zmk-physical-layout-converter.streamlit.app/).

For more details on how to use ZMK Studio, refer to the [ZMK documentation](https://zmk.dev/docs/features/studio).

**Keymap GUI**

Using a GUI to generate the keymap file before building the firmware is another easy way to modify the key mappings. Head over to nickcoutsos' keymap editor and follow the steps below.

- Fork/Clone this repo
- Open a new tab to the [keymap editor](https://nickcoutsos.github.io/keymap-editor/)
- Give it permission to see your repo
- Select the branch you'd like to modify
- Update the keys to match what you'd like to use on your keyboard
- Save
- Wait for the pipeline to run
- Download and flash the new firmware

**Edit Keymap Directly**

To change a key layout choose a behavior you'd like to assign to a key, then choose a parameter code. This process is more clearly outlined on ZMK's [Keymaps & Behaviors](https://zmk.dev/docs/features/keymaps) page. All keycodes are documented [here](https://zmk.dev/docs/codes) page

Open the config/keymap/charybdis.keymap file and change keys, or add/remove layers, then merge the changes and re-flash the keyboard with the updated firmware.


### Modifying Trackball Behavior

The trackball uses ZMK's modular input processor system, making it easy to adjust pointer behavior to your liking. All trackball-related configurations and input processors are conveniently grouped in the `config/charybdis_pointer.dtsi` file. Modify this file to customize tracking speed, acceleration, scrolling behavior, and more—then rebuild your firmware.


### Building Your Customized Firmware

You can easily build your firmware locally or leverage GitHub Actions:

**Local Build (recommended for quick testing and debugging)**

Clone this repo, then run these commands from the repo root:
```sh
cd local-build
docker-compose run --rm builder
```
See the [local build README](local-build/README.md) for additional details, including how to enable USB logging in the builds.

**GitHub Actions**

- Fork or clone this repo
- Push your changes to your GitHub
- GitHub Actions automatically builds your firmware and publishes downloadable artifacts under the Actions tab.


### Troubleshooting

- If the keyboard halves aren't connecting as expected, try pressing the reset button on both halves at the same time. If that doesn't work, follow the [ZMK Connection Issues](https://zmk.dev/docs/troubleshooting/connection-issues#acquiring-a-reset-uf2) documentation for more troubleshooting steps.
- If you run into a bug or something’s not working, feel free to open an issue or submit a PR! Just keep in mind I'm not a developer, and this is a hobby project so I may not be able to fix everything.


## Credits

- [badjeff](https://github.com/badjeff)
- [eigatech](https://github.com/eigatech)
- [nickcoutsos](https://github.com/nickcoutsos/keymap-editor)
- [caksoylar](https://github.com/caksoylar/keymap-drawer)
- [urob](https://github.com/urob/zmk-config#timeless-homerow-mods)