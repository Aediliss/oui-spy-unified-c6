/*
 * Mode 3: Flock-You — Promiscuous WiFi Edition
 *
 * Passive 2.4 GHz promiscuous-mode detector for Flock Safety surveillance
 * infrastructure. Wraps the standalone firmware from the `promiscious`
 * branch of colonelpanichacks/flock-you in an anonymous namespace.
 *
 * Detection methods (WiFi only — no AP, no BLE):
 *   - addr2 OUI match  (transmitter-side, @NitekryDPaul list)
 *   - addr1 OUI match  (receiver-side, @NitekryDPaul's sleeper-catch)
 *   - wildcard probe   (probe req + zero-length SSID + known OUI, the
 *                       DeFlockJoplin high-precision signature)
 *
 * Outputs:
 *   - Live Flask-compatible JSON over USB-CDC (one line per detection)
 *   - SPIFFS-persisted session with CRC envelope; the host can pull it
 *     back via the CMD:* protocol (CMD:DUMP_PREV / CMD:DUMP_LIVE etc.)
 */

// All includes from the original firmware must be OUTSIDE the namespace
// so they get external linkage. Re-inclusions inside the namespace are
// no-ops thanks to header guards.
#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <ctype.h>
#include <string.h>
#include <SPIFFS.h>
#include "modes.h"
#include "compat_esp32c6.h"   // C6: buzzer/pin/single-core shim (no-op on S3)

// Rename setup/loop so they don't collide with the unified main.cpp's
// Arduino entry points (and the other modes' wrapped setup/loop).
#define setup flockyou_promiscious_ns_setup
#define loop  flockyou_promiscious_ns_loop

namespace {
#include "raw/flockyou_promiscious.cpp"
} // anonymous namespace

#undef setup
#undef loop

void flockyou_promiscious_setup() { flockyou_promiscious_ns_setup(); }
void flockyou_promiscious_loop()  { flockyou_promiscious_ns_loop(); }
