# EPOMAKER Split65

## 🎹 Overview

Custom QMK firmware for the **EPOMAKER Split65**, a split 65% keyboard with advanced wireless features.

---

## 🏗️ Hardware Architecture

| Element | Detail |
|---|---|
| **MCU** | WB32FQ95 (Westberry Technology) |
| **Bootloader** | wb32-dfu |
| **Matrix** | 12 rows × 9 columns (ROW2COL) |
| **Split link** | USART full-duplex (TX: A9, RX: A10, 115 200 baud, driver SD1) |
| **Handedness detection** | Pin B9 (`SPLIT_HAND_PIN`) |
| **Rotary encoder** | Right side (pins B7 / B6) |
| **RGB LEDs** | WS2812 via SPI (pin B15, driver `SPIDM2`) — **68 LEDs** |
| **SPI bus** | SCK B3 · MOSI B5 · MISO B4 (driver `SPIDQ`) |
| **External flash** | SPI CS pin C12 — wear leveling (8 KB backing / 4 KB logical) |
| **Wireless UART** | SD3 (TX C10, RX C11) — wireless module communication |

---

## 📡 Wireless Connectivity (`WIRELESS_ENABLE`)

Managed via `linker/wireless/` and `wls/`:

- **Bluetooth**: 3 profiles (`BT1`, `BT2`, `BT3`) — names `Split65-1 / 2 / 3`
- **2.4 GHz**: Dedicated dongle (`2.4G Dongle`)
- **Wired USB**: Classic HID mode
- Automatic mode detection via dedicated pins (C14 for BT, C15 for 2.4 GHz)
- **Battery management**: LED indicator, charge state (pins A7, A15), low threshold at 15%
- **Low-power mode** (`lowpower.c/h`, `lpwr_wb32.c`)

---

## 🌈 RGB Matrix

- **68 LEDs** across both halves
- **44 animations** enabled (breathing, cycle, raindrops, heatmap, digital rain, pixel effects, reactive, etc.)
- Dedicated visual indicators:
  - Caps Lock — LED index **2**
  - Win Lock — LED index **1**
  - Win layer blink — LED index **15**
  - Mac layer blink — LED index **14**
  - BT1 / BT2 / BT3 / 2.4 GHz mode — LED indices **17 / 18 / 19 / 20** (colored blink)
  - Battery level — 10 LEDs at indices **{27, 26, 25, 24, 23, 22, 29, 30, 31, 32}**
- **RGB Record** (`rgb_record/`): records custom per-key RGB patterns, 4 channels, stored in EEPROM

---

## ⌨️ Keymaps (4 layers)

| Layer | Name | Description |
|---|---|---|
| `_BL` | Windows Base | Standard QWERTY |
| `_FL` | Windows Fn | F1–F12, RGB control, BT/2.4G switching, battery |
| `_MBL` | macOS Base | QWERTY with Cmd/Option swapped |
| `_MFL` | macOS Fn | Same as `_FL`, adapted for macOS |

Notable special keycodes:
- `KC_BT1` / `KC_BT2` / `KC_BT3` — select Bluetooth profile
- `KC_2G4` — switch to 2.4 GHz mode
- `KC_FILP` — flip layout direction
- `HS_BATQ` / `KC_BATQ` — display battery status via LED indicators
- `RL_MOD` — cycle through RGB lighting modes
- `EE_CLR` — reset EEPROM (hold to confirm)

---

## 💾 EEPROM Storage

Wear leveling on external SPI flash (8 KB backing / 4 KB logical):

| Block | Size | Offset |
|---|---|---|
| **RGB Record** | `RGBREC_CHANNEL_NUM × MATRIX_ROWS × MATRIX_COLS × 2 bytes` = 864 B (4 ch) | 0 |
| **confinfo** | `4 + 16 = 20 bytes` (mode, layer, direction flags, etc.) | after RGB Record |

---

## 📁 Module Structure

```
split65.c          → Main firmware logic (init, RGB indicators, split sync, key handling)
config.h           → Full hardware / feature configuration
keyboard.json      → QMK definition (matrix, layout, RGB, USB, split, encoder)
wls/               → Wireless mode management (BT / 2.4G / USB detection)
linker/wireless/   → Low-level wireless stack (transport, module, low-power)
rgb_record/        → RGB sequence recording feature
keymaps/via/       → Keymap with VIA support (dynamic remapping)
keymaps/default/   → Default keymap
```

---

## 🔧 Notable Technical Points

1. **Split synchronisation**: uses `SPLIT_TRANSACTION_IDS_USER` (`USER_SYNC_MMS`) to synchronise multi-mode state (BT/2.4G) between halves over USART.
2. **VIA compatible**: keymap and EEPROM sized for dynamic remapping via the VIA interface.
3. **Dual-MCU split**: each half has its own matrix pin set — left: `C0–B11` (7 cols + 2 null), right: `B12–C5` (9 cols); both share rows `A0–C13`.
4. **USB VID/PID**: `0x342D:0xE4C6`, firmware version `0.3.0`.
5. **Auto sleep**: 5-minute inactivity timeout (`HS_SLEEP_TIMEOUT`) in wireless mode.

---

## 🔗 References

- [QMK Firmware](https://docs.qmk.fm/)
- [GitHub Repository](https://github.com/alekart/Split65) - [Forked from gwangyi](https://github.com/gwangyi/Split65)
- [EPOMAKER GitHub](https://github.com/Epomaker)
