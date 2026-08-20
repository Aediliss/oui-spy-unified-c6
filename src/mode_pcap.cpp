/*
 * Mode 4: PCAP — Passive WiFi Packet Capture
 *
 * Wraps the merged colonelpanichacks/ouispy-pcap firmware in an anonymous
 * namespace so its symbols (config/capture/pcap_stream/session_pcap/
 * text_summary/web_dashboard) don't collide with any other mode's code.
 *
 * Fills the mode-4 slot vacated by the retired Flock-You BLE mode.
 */

// All includes the merged file needs live OUTSIDE the anonymous namespace so
// they get external linkage. Re-inclusions inside the namespace are no-ops
// thanks to header guards.
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <driver/ledc.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <sys/time.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "modes.h"
#include "compat_esp32c6.h"   // C6: buzzer/pin/single-core shim (no-op on S3)

// Rename setup/loop so they don't collide with the Arduino entry points in
// main.cpp or the other modes' wrapped setup/loop.
#define setup pcap_ns_setup
#define loop  pcap_ns_loop

namespace {
#include "raw/pcap.cpp"
} // anonymous namespace

#undef setup
#undef loop

void pcap_setup() { pcap_ns_setup(); }
void pcap_loop()  { pcap_ns_loop(); }
void pcap_stop() {
    // main.cpp reboots back to the selector on BOOT hold, so this only runs
    // in the (currently unused) hot-swap path via ModeManager. Just kill the
    // radio; the async web server + writer tasks unwind cleanly on restart.
    esp_wifi_set_promiscuous(false);
}
