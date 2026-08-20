# OUI Spy Unified Blue – Multi-Mode BLE/WiFi Surveillance Detection Firmware

Six-mode unified firmware for Seeed Studio XIAO ESP32-S3, XIAO ESP32-C6, and Heltec WiFi LoRa 32 V4, with boot selector menu. Detects surveillance hardware, drones, and BLE tracking devices.

## Hardware

Three targets. The C6 port lives in `src/compat_esp32c6.h` (active only when
`CONFIG_IDF_TARGET_ESP32C6`) + `src/nimble_compat_c6.h` + `#ifndef`-guarded pin
defines. The Heltec V4 is another ESP32-S3, so it reuses the S3 code path with
no shims — just a new env + pins injected via `-D` flags. The XIAO-S3 build is
unchanged. See `PORTING_ESP32C6.md` and the `platformio.ini` env comments.

| Signal | XIAO ESP32-S3 | XIAO ESP32-C6 | Heltec WiFi LoRa 32 V4 |
|---|---|---|---|
| Buzzer (PWM) | GPIO 3 | GPIO 2 (D2) | GPIO 7 (external only) |
| LED (inverted logic) | GPIO 21 (HIGH=OFF) | GPIO 15 (onboard) | GPIO 35 (onboard, active-**high** → reads inverted) |
| Boot button (hold 1.5–2s) | GPIO 0 | GPIO 9 (onboard) | GPIO 0 (PRG) |
| NeoPixel (Detector) | GPIO 4 | GPIO 1 (D1) | GPIO 4 (external only) |
| Flock-You Serial1 mirror | GPIO 43 | GPIO 20 (D9) | GPIO 43 |

- C6: single-core RISC-V @ 160 MHz, **no PSRAM**, 4 MB flash, USB-Serial-JTAG.
  Arduino core 3.x required (pioarduino platform).
- Heltec V4: ESP32-S3 @ 240 MHz, **2 MB quad PSRAM** (`qio_qspi`), 16 MB flash,
  native USB. Core 2.x (espressif32), same as the XIAO S3. GPIO21 is the OLED
  reset here (not an LED); no onboard buzzer/NeoPixel. All modes tested on-device.
- Build/flash test hook: `-DOUISPY_FORCE_MODE=<n>` boots straight into mode n
  (bypassing the web selector) and prints PSRAM/heap — inert in normal builds.

## Modes

| Mode | File | Purpose |
|------|------|---------|
| Boot Selector | `src/main.cpp` | Mode selection menu via serial/web |
| Detector | `src/raw/detector.cpp` | OUI-based WiFi surveillance detection |
| Foxhunter | `src/raw/foxhunter.cpp` | RSSI proximity tracker for specific BLE targets |
| Flock-You | `src/raw/flockyou.cpp` | Surveillance detection with GPS logging |
| Sky Spy | `src/raw/skyspy.cpp` | Drone Remote ID detection (BLE + WiFi) |
| BLE Sniff | `src/raw/blesniff.cpp` | Passive BLE advertising capture (Wireshark-ready) |

## Build & Run

```bash
# C6 (default env)
pio run -e seeed_xiao_esp32c6
pio run -e seeed_xiao_esp32c6 -t upload
# S3
pio run -e seeed_xiao_esp32s3
pio run -e seeed_xiao_esp32s3 -t upload
# Heltec WiFi LoRa 32 V4
pio run -e heltec_wifi_lora_32_v4
pio run -e heltec_wifi_lora_32_v4 -t upload
pio device monitor -b 115200
```

## PlatformIO Config

- **Board**: `seeed_xiao_esp32s3`
- **Partition**: Custom (`partitions.csv`) — 6MB app, 2MB LittleFS
- **BLE**: NimBLE-Arduino 1.4.0+
- **Web**: AsyncWebServer 3.0.6+
- **Filesystem**: LittleFS (web assets, config)

## Build Flags

```
-DCORE_DEBUG_LEVEL=0
-DARDUINO_USB_CDC_ON_BOOT=1
-DBOARD_HAS_PSRAM
-mfix-esp32-psram-cache-issue
-DCONFIG_BT_NIMBLE_ENABLED=1
-Isrc/raw
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

## Web Interface

- **AP IP**: 192.168.4.1 (all modes)
- Served from LittleFS partition
- Mode-specific endpoints for config and data export

## Architecture

- Anonymous namespaces for symbol isolation between modes
- Each mode is a self-contained `.cpp` file in `src/raw/`
- Boot selector in `main.cpp` routes to selected mode
- ~9700 lines total across 4 firmware implementations

## Gotchas

1. **LED inverted**: `digitalWrite(21, HIGH)` = OFF, `LOW` = ON
2. **Boot sound**: Zelda "Secret Discovered" jingle on startup
3. **Buzzer frequencies**: 1000 Hz (low alert), 2000 Hz (general), 3000 Hz (high alert) — avoid <20 Hz
4. **Flock-You memory**: Max 200 unique detections, oldest overwritten after that — export regularly
5. **Sky Spy dual capture**: Both BLE (UUID 0xFAFF) and WiFi action frames — channel swap window may miss some
6. **GPIO conflicts**: Don't reassign GPIO 0, 3, 21, 43, 44
7. **AsyncWebServer + NVS**: NVS writes are synchronous — can cause brief freezes in HTTP handlers
8. **PSRAM cache fix**: `-mfix-esp32-psram-cache-issue` build flag is critical
9. **Mode persistence**: Selected mode saved to NVS, survives reboot
10. **Flash utility**: `flash.py` for automated flashing
