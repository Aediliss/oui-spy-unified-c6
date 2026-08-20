# OUI-SPY on the Seeed XIAO ESP32-C6

This firmware was written for the Seeed XIAO **ESP32-S3**. This repo adds a
second build target, the Seeed XIAO **ESP32-C6**, without disturbing the S3
build. All six modes (Detector, Foxhunter, Flock-You WiFi, PCAP, Sky Spy, BLE
Sniff) and the boot selector are included.

> **Status:** ✅ **Builds, flashes, and runs on a real XIAO ESP32-C6.** Verified
> with PlatformIO + pioarduino (Arduino-ESP32 core 3.3.11, ESP-IDF 5.5,
> NimBLE 2.5.1, ESPAsyncWebServer 3.12.0 / AsyncTCP 3.5.0): `pio run
> -e seeed_xiao_esp32c6 -t upload` → SUCCESS. RISC-V image, chip id `0x000D`;
> **RAM 28.8%, Flash 59.3%** (1.85 MB app in the 2.875 MB partition). On the
> device it boots cleanly, starts the `oui-spy` AP at 192.168.4.1, brings the
> web server up, and reaches steady state (no boot loop). Prebuilt binaries are
> in `firmware-c6/`. BLE-scanning behaviour under NimBLE 2.x is worth a spot
> check on your specific targets, but the firmware itself runs.

---

## Why the C6 needs a port at all

The C6 is not a drop-in for the S3. Four hardware/toolchain differences drive
every change in this port:

| | XIAO ESP32-S3 | XIAO ESP32-C6 |
|---|---|---|
| CPU | dual-core Xtensa LX7 @ 240 MHz | **single-core** RISC-V @ **160 MHz** |
| PSRAM | 8 MB | **none** |
| Flash | 8 MB | **4 MB** |
| USB | native USB-OTG | **USB-Serial-JTAG** |
| Arduino core | 2.x (stock PlatformIO) | **3.x only** (C6 unsupported before 3.0) |

Jumping to Arduino-ESP32 **core 3.x** (the first core with C6 support) is the
real work: core 3.x removed the channel-based LEDC (buzzer) API and the single
core makes `xTaskCreatePinnedToCore(..., 1)` illegal. On top of that, the C6's
Bluetooth controller is only supported by **NimBLE-Arduino 2.x**, whose scan
API differs from the 1.4.x API the modes were written against.

---

## What changed

Everything C6-specific is isolated so the S3 build is byte-for-byte unchanged.

### `src/compat_esp32c6.h` (new) — the whole port in one header
Active only when building for the C6 (`CONFIG_IDF_TARGET_ESP32C6`); a complete
no-op on every other target. It handles:

1. **LEDC buzzer API (2.x → 3.x).** Core 3.x deleted `ledcSetup` /
   `ledcAttachPin` and made `ledcWrite` / `ledcWriteTone` take a *pin* instead
   of a channel. The whole firmware drives one buzzer on LEDC channel 0, so the
   shim re-implements those channel-0 calls on top of the new pin-based API —
   including mid-melody frequency changes (`ledcChangeFrequency`), so the boot
   jingles still play correctly.
2. **Single core.** `xTaskCreatePinnedToCore(..., core)` is wrapped so any core
   id the C6 doesn't have (i.e. core 1) becomes `tskNO_AFFINITY` instead of
   asserting at boot.
3. **Pin map.** Publishes the XIAO ESP32-C6 pins (below). The mode sources pick
   them up because their own pin `#define`s are now `#ifndef`-guarded.

### Small guard edits
Each hard-coded pin `#define` in `src/main.cpp`, `src/raw/config.h`, and the
compiled `src/raw/*.cpp` mode sources was wrapped in `#ifndef … #endif` so the
shim's C6 values win, while the original S3 numbers remain the default. Each of
the six `src/mode_*.cpp` wrappers (and `main.cpp`) now `#include
"compat_esp32c6.h"`. No mode *logic* was touched.

### `src/nimble_compat_c6.h` (new) — the NimBLE 1.4.x → 2.x bridge
The four BLE modes are written against NimBLE 1.4.x, but the C6 needs 2.x. This
header (active on the C6, inert on the S3) bridges the API differences so the
same mode sources build against both:
- `NimBLEAdvertisedDeviceCallbacks` (renamed to `NimBLEScanCallbacks` in 2.x,
  with a `const` `onResult`) — re-declared as a thin adapter that forwards to
  the old non-const `onResult` the modes implement.
- `setAdvertisedDeviceCallbacks` → `setScanCallbacks`, and
  `getInitialized` → `isInitialized` — macro renames.
- `start()` now takes **milliseconds** and returns `bool`; `getPayload()`
  returns a `std::vector`; `getPayloadLength()` is gone; `getNative()` →
  `getVal()` — bridged by the `OUISPY_BLE_*` helper macros, which are defined
  for both NimBLE versions so the (lightly edited) call sites are identical on
  S3 and C6. `platformio.ini` pins NimBLE per-env: S3 → 1.4.x, C6 → 2.x.

### Async web stack — esp32async fork on the C6
The C6 SDK enables lwIP TCPIP **core-locking** (`CONFIG_LWIP_TCPIP_CORE_LOCKING=y`,
`CONFIG_LWIP_CHECK_THREAD_SAFETY=y`). The old AsyncTCP 3.1.4 calls `tcp_alloc()`
without holding that lock, so the device *compiled fine but crashed on boot* at
`server.begin()` with `assert failed: tcp_alloc ... Required to lock TCPIP core
functionality!`, boot-looping. Fix: the C6 env uses the maintained
**esp32async** fork — `esp32async/ESPAsyncWebServer@^3.12.0` +
`esp32async/AsyncTCP@^3.5.0` — which takes the core lock. (The S3 env keeps
`mathieucarbou/...@3.0.6 / 3.1.4`, which is correct for its older SDK.) This is
the one bug that only surfaced on hardware, not at compile time.

### `platformio.ini` — now dual-target
- `env:seeed_xiao_esp32s3` — the original, unchanged (NimBLE 1.4.x, mathieucarbou async).
- `env:seeed_xiao_esp32c6` — new. Uses the **pioarduino** platform (Seeed's own
  platform is a commented alternative — it reports its name as `SeeedStudio`,
  which trips the compat gate on the `espressif32`-only ESPAsyncWebServer/
  AsyncTCP libraries). Pins NimBLE 2.x, lists AsyncTCP explicitly, drops the
  PSRAM flags and the Xtensa-only `-mfix-esp32-psram-cache-issue`, sets CPU to
  160 MHz, uses the 4 MB partition table, and adds `-DARDUINO_USB_MODE=1` so
  `Serial` stays on the USB-Serial-JTAG port. `pio run` builds the C6 by default.

### `partitions_4mb.csv` (new)
4 MB layout for the C6: 2.8 MB app (the image is ~1.2 MB) + ~1 MB SPIFFS for
Flock-You's session persistence. The S3 keeps its 8 MB `partitions.csv`.

### `flash.py` — `--c6` flag; `firmware-c6/` drop folder
`python flash.py --c6` targets chip `esp32c6` / 4 MB from `firmware-c6/`.

### PSRAM: no code change needed
The PCAP and BLE Sniff modes already fall back gracefully when no PSRAM is
present — the 2 MB in-PSRAM session-download buffer becomes a **64 KB
internal-RAM** buffer, and the capture ring buffers use fewer slots. **Live USB
pcap streaming and the web dashboards are unaffected**; only the size of the
"download the whole session" buffer shrinks. Nothing crashes; the modes just
hold less history in RAM.

---

## Wiring (XIAO ESP32-C6)

The S3's onboard LED (GPIO21) and BOOT button (GPIO0) map cleanly to the C6's
onboard LED (GPIO15, active-low — same inverted logic) and BOOT button (GPIO9).
The external buzzer and the optional Detector NeoPixel move to broken-out pads:

| Function | GPIO | XIAO pad | Notes |
|---|---|---|---|
| Piezo buzzer | 2 | **D2** | all modes |
| NeoPixel (Detector, optional) | 1 | **D1** | external WS2812 |
| User LED | 15 | onboard | active-low, no wiring |
| BOOT button | 9 | onboard | hold 1.5 s → selector |
| Flock-You Serial1 debug mirror | 20 | **D9** | optional, TX-only |

Using a different C6 board (e.g. ESP32-C6-DevKitC)? Change the pins in one place
— the `#ifndef` block near the top of `src/compat_esp32c6.h`.

---

## Building

The C6 needs Arduino-ESP32 **core 3.x**, which the stock PlatformIO
`espressif32` platform doesn't ship. `platformio.ini` defaults to the
**pioarduino** platform (the tested toolchain for this port); Seeed's own
platform is a commented alternative. The first build pulls ~1–2 GB of core +
RISC-V toolchain.

```bash
# build (C6 is the default env)
pio run -e seeed_xiao_esp32c6

# build + flash directly over USB (simplest path)
pio run -e seeed_xiao_esp32c6 -t upload

# serial monitor (USB-Serial-JTAG)
pio device monitor -e seeed_xiao_esp32c6 -b 115200
```

The S3 build is still there: `pio run -e seeed_xiao_esp32s3`.

### Flashing prebuilt bins with flash.py
`firmware-c6/` is populated with a verified build (bootloader, partitions,
boot_app0, and the app). To flash those without PlatformIO:

```bash
python flash.py --c6
```

After changing the firmware, rebuild and refresh them (see
`firmware-c6/README.txt`), or just use `pio run -e seeed_xiao_esp32c6 -t upload`.

---

## Runtime behaviour vs. the S3

- **Everything works the same** at the UI/feature level: boot selector AP,
  per-mode APs and dashboards, buzzer jingles, USB-CDC pcap/JSON, SPIFFS
  persistence for Flock-You.
- **Smaller session-download buffers** in PCAP / BLE Sniff (64 KB vs 2 MB) —
  export more often if you need long histories. Live streaming is unchanged.
- **160 MHz vs 240 MHz** and a single core: promiscuous capture throughput is
  lower than the S3's. For the heaviest WiFi capture, the S3 remains the
  stronger board.
- **Wi-Fi:** the C6 is Wi-Fi 6 (2.4 GHz) — promiscuous mode and channel hopping
  work as before.

---

## Files added / touched by the port

```
added:    src/compat_esp32c6.h        C6 compat shim (buzzer/core/pins)
added:    src/nimble_compat_c6.h      NimBLE 1.4.x -> 2.x scan-API bridge
added:    partitions_4mb.csv          4 MB flash layout for the C6
added:    firmware-c6/                verified C6 binaries + flash.py drop folder
added:    PORTING_ESP32C6.md          this document
changed:  platformio.ini              dual-target; per-env NimBLE; pioarduino C6
changed:  flash.py                    --c6 flag
changed:  src/main.cpp                #ifndef-guard pins, include compat shim
changed:  src/mode_*.cpp (x6)         include compat shim (BLE ones + NimBLE shim)
changed:  src/mode_manager.cpp        include NimBLE shim (getInitialized)
changed:  src/raw/config.h            #ifndef-guard BUZZER_PIN
changed:  src/raw/{detector,foxhunter,skyspy,flockyou_promiscious,pcap,blesniff}.cpp
                                      #ifndef-guard pins; NimBLE 2.x call-site bridges
```
