/*
 * Mode 6: BLE SNIFF - Passive BLE Advertising Capture
 *
 * Wraps the merged colonelpanichacks/ouispy-blesniff firmware in an anonymous
 * namespace so its symbols (config/scan/nordic_pcap/pcap_stream/session_pcap/
 * text_summary/web_dashboard) don't collide with any other mode's code.
 *
 * Slot: right after Sky Spy (mode 6). LINKTYPE_BLUETOOTH_LE_LL_WITH_PHDR
 * pcap over USB-CDC + text summary, dashboard at ouispy-blesniff /
 * sniffuntothem, 2 MB in-PSRAM session pcap.
 */

// All includes the merged file needs live OUTSIDE the anonymous namespace so
// they get external linkage. Re-inclusions inside the namespace are no-ops
// thanks to header guards.
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <driver/ledc.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <sys/time.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "modes.h"

// Rename setup/loop so they don't collide with the Arduino entry points in
// main.cpp or the other modes' wrapped setup/loop.
#define setup blesniff_ns_setup
#define loop  blesniff_ns_loop

namespace {
#include "raw/blesniff.cpp"
} // anonymous namespace

#undef setup
#undef loop

void blesniff_setup() { blesniff_ns_setup(); }
void blesniff_loop()  { blesniff_ns_loop(); }
void blesniff_stop() {
    // main.cpp reboots back to the selector on BOOT hold, so this only runs
    // in the (currently unused) hot-swap path via ModeManager. Best-effort
    // teardown; the async web server + writer tasks unwind cleanly on restart.
    if (NimBLEDevice::getInitialized()) {
        NimBLEScan* s = NimBLEDevice::getScan();
        if (s) s->stop();
    }
}

// OLED headline stat: total BLE advertisements captured (reuses the counter).
uint32_t blesniff_stat() { return scan::total_adverts(); }
