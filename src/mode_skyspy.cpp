/*
 * Mode 5: Sky Spy - Open Drone ID Detector
 * Monitors WiFi and BLE for FAA Remote ID broadcasts from drones.
 * Ported from classic ESP32 BLE to NimBLE via compatibility macros.
 * Wraps the original Sky-Spy firmware in an anonymous namespace.
 */

// All includes (outside namespace) - NimBLE instead of classic BLE
#include <Arduino.h>
#include <HardwareSerial.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include "opendroneid.h"
#include "odid_wifi.h"
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Preferences.h>
#include "modes.h"
#include "compat_esp32c6.h"   // C6: buzzer/pin/single-core shim (no-op on S3)
#include "nimble_compat_c6.h"  // C6: NimBLE 2.x scan-API bridge (no-op on S3)

// Rename setup/loop
#define setup skyspy_ns_setup
#define loop  skyspy_ns_loop

namespace {
#include "raw/skyspy.cpp"
} // anonymous namespace

#undef setup
#undef loop

void skyspy_setup() { skyspy_ns_setup(); }
void skyspy_loop()  { skyspy_ns_loop(); }
