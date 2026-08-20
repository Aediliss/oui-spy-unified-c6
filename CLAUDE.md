# OUI Spy Unified Blue – Heltec WiFi LoRa 32 V4

Six-mode BLE/WiFi surveillance-detection firmware for the **Heltec WiFi LoRa 32
V4** (ESP32-S3), with a boot-selector menu and an on-board OLED status display.
Detects surveillance hardware, drones, and BLE tracking devices. Fork of
colonelpanichacks's XIAO ESP32-S3 project, ported to the Heltec V4.

## Hardware

Single target: Heltec WiFi LoRa 32 V4 — ESP32-S3 @ 240 MHz, **2 MB quad PSRAM**
(`qio_qspi`), 16 MB flash, native USB (USB-Serial-JTAG), on-board SSD1306 OLED
and SX1262 LoRa (LoRa unused). Pins are injected via `-D` flags in
`platformio.ini` over `#ifndef`-guarded defaults in the mode sources.

| Signal | GPIO | Notes |
|---|---|---|
| On-board LED | 35 | active-**high** → reads inverted (lit at idle) |
| Boot/PRG button | 0 | hold 1.5–2 s → selector |
| OLED (I2C) | SDA 17 / SCL 18 / RST 21 | powered via Vext (GPIO 36) |
| Buzzer (PWM) | 7 | **external only** — no on-board buzzer |
| NeoPixel (Detector) | 4 | **external only** — no on-board NeoPixel |
| Flock-You Serial1 mirror | 43 | U0TXD pad |

- No on-board buzzer/NeoPixel → audible/RGB alerts silent unless external parts
  are wired; the OLED is the local feedback. GPIO 21 is the OLED reset, not an
  LED (hence the LED on 35).
- Test hook: `-DOUISPY_FORCE_MODE=<n>` boots straight into mode n (bypassing the
  web selector) and prints PSRAM/heap — inert in normal builds.

## Modes

| Mode | File | Purpose |
|------|------|---------|
| Boot Selector | `src/main.cpp` | Mode selection menu via serial/web |
| Detector | `src/raw/detector.cpp` | OUI-based BLE surveillance detection |
| Foxhunter | `src/raw/foxhunter.cpp` | RSSI proximity tracker for specific BLE targets |
| Flock-You | `src/raw/flockyou_promiscious.cpp` | Promiscuous WiFi Flock Safety detection |
| Sky Spy | `src/raw/skyspy.cpp` | Drone Remote ID detection (BLE + WiFi) |
| BLE Sniff | `src/raw/blesniff.cpp` | Passive BLE advertising capture (Wireshark-ready) |

## On-board OLED

`src/oled_status.{h,cpp}` drives a 128×64 SSD1306 status screen (mode, uptime,
connection, and a live per-mode stat). Driven from `main.cpp` (`oled_begin` in
setup, `oled_tick` ~1 Hz in loop). Each mode exposes a small stat getter
declared in `src/modes.h` and defined in its `src/mode_*.cpp` wrapper (reading
the mode's anonymous-namespace counter): `detector_stat` (devices),
`foxhunter_stat` (target RSSI, +bar), `flockyou_stat`, `pcap_stat`,
`skyspy_stat` (drones), `blesniff_stat`.

## Build & Run

```bash
pio run                       # build (single default env)
pio run -t upload             # build + flash over USB
pio device monitor -b 115200  # serial
```

## PlatformIO Config

- **Platform/board**: `espressif32@^6.3.0` (Arduino core 2.x), board `heltec_wifi_lora_32_V3` (V4 is pin-compatible)
- **Partition**: `partitions_16mb.csv` — 6 MB app, 2 MB SPIFFS (SPIFFS kept small; unreliable on large partitions)
- **PSRAM**: `board_build.arduino.memory_type = qio_qspi` (2 MB quad — not octal)
- **BLE**: NimBLE-Arduino 1.4.x · **Web**: mathieucarbou AsyncWebServer 3.0.6 + AsyncTCP 3.1.4 · **OLED**: Adafruit SSD1306

## Build Flags

```
-DCORE_DEBUG_LEVEL=0
-DARDUINO_USB_CDC_ON_BOOT=1
-DARDUINO_USB_MODE=1
-DBOARD_HAS_PSRAM
-DCONFIG_BT_NIMBLE_ENABLED=1
-Isrc/raw
-DLED_PIN=35 -DPCAP_LED_PIN=35 -DBLESNIFF_LED_PIN=35
-DBUZZER_PIN=7 -DPCAP_BUZZER_PIN=7 -DBLESNIFF_BUZZER_PIN=7
-DNEOPIXEL_PIN=4
```

## NVS Namespaces

| Namespace | Purpose |
|-----------|---------|
| `unified-mode` | Selected boot mode |
| `ouispy-ap` | AP credentials |
| `ouispy-bz` | Buzzer toggle |
| `pcap-mode` | PCAP mode config |
| `blesniff` | BLE Sniff mode config |

Don't reuse these in mode code.

## Architecture

- Each mode is a self-contained `.cpp` in `src/raw/`, wrapped by a `src/mode_*.cpp`
  that `#include`s it inside an **anonymous namespace** for symbol isolation.
- `src/main.cpp` boot-selector routes to the selected mode; mode chosen via the
  web selector (AP `oui-spy` @ 192.168.4.1) and saved to NVS.
- Web dashboards are embedded in firmware (PROGMEM), served per-mode.

## Gotchas

1. **LED**: GPIO 35, active-**high** on the Heltec — the code drives it active-low, so it reads inverted (lit at idle). Cosmetic.
2. **GPIO 21 is the OLED reset**, not an LED. OLED I2C on 17/18; USB on 19/20; Vext on 36. Don't reassign 0/17/18/19/20/21/35/36.
3. **No on-board buzzer/NeoPixel** — buzzer (7) / NeoPixel (4) need external parts; a benign `ledc: LEDC is not initialized` may print at boot from the tone() path.
4. **PSRAM**: `qio_qspi` (2 MB quad). Octal (`qio_opi`) would clash with GPIO 33–37 (the LED).
5. **SPIFFS**: keep the partition ≤ 2 MB; large SPIFFS mounts unreliably. First-boot `-10025` is the expected format-then-mount.
6. **Flock-You memory**: max 200 unique detections, oldest overwritten — export regularly.
7. **AsyncWebServer + NVS**: NVS writes are synchronous — brief freezes possible in HTTP handlers.
8. **Mode persistence**: selected mode saved to NVS, survives reboot.
9. **Flash utility**: `flash.py` (defaults to esp32s3 / 16 MB / `firmware-heltec/`).
