/*
 * OUI SPY - on-board OLED status display implementation (Heltec V4).
 */
#include "oled_status.h"
#include "modes.h"           // per-mode stat getters

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_SSD1306.h>

// These come from the Heltec board's pins_arduino.h; fall back to the V3/V4
// values so the module is robust if the variant macro names ever differ.
#ifndef SDA_OLED
#define SDA_OLED 17
#endif
#ifndef SCL_OLED
#define SCL_OLED 18
#endif
#ifndef RST_OLED
#define RST_OLED 21
#endif
#ifndef Vext
#define Vext 36
#endif

#define OLED_W 128
#define OLED_H 64

static Adafruit_SSD1306 g_disp(OLED_W, OLED_H, &Wire, RST_OLED);
static bool g_ok = false;

static const char* mode_name(int m) {
    switch (m) {
        case 0: return "SELECTOR";
        case 1: return "DETECTOR";
        case 2: return "FOXHUNTER";
        case 3: return "FLOCK-YOU";
        case 4: return "PCAP";
        case 5: return "SKY SPY";
        case 6: return "BLE SNIFF";
        default: return "UNKNOWN";
    }
}

void oled_begin() {
    // Vext is active-low on the Heltec — pull it low to power the OLED rail.
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
    delay(50);

    Wire.begin(SDA_OLED, SCL_OLED);
    // periphBegin=false: we already called Wire.begin() with the OLED pins.
    g_ok = g_disp.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, false);
    if (!g_ok) g_ok = g_disp.begin(SSD1306_SWITCHCAPVCC, 0x3D, true, false);
    Serial.printf("[OLED] SSD1306 %dx%d begin: %s\n", OLED_W, OLED_H,
                  g_ok ? "OK" : "FAILED (no I2C ACK)");
    if (!g_ok) return;

    g_disp.clearDisplay();
    g_disp.setTextColor(SSD1306_WHITE);
    g_disp.setTextSize(2);
    g_disp.setCursor(0, 4);
    g_disp.println("OUI-SPY");
    g_disp.setTextSize(1);
    g_disp.setCursor(0, 26);
    g_disp.println("Heltec WiFi");
    g_disp.println("LoRa 32 V4");
    g_disp.display();
}

// Render the live headline stat for `mode` at the cursor's current row.
// Returns true if it drew a Foxhunter RSSI value (caller then draws the bar).
static bool draw_stat(int mode, int* rssi_out) {
    char buf[24];
    switch (mode) {
        case 1: snprintf(buf, sizeof(buf), "Devices: %d", detector_stat()); break;
        case 2: {
            int r = foxhunter_stat();
            if (r > 0) { g_disp.print("No target set"); return false; }
            snprintf(buf, sizeof(buf), "RSSI: %d dBm", r);
            g_disp.print(buf);
            if (rssi_out) *rssi_out = r;
            return true;   // caller draws the signal bar
        }
        case 3: snprintf(buf, sizeof(buf), "Flock hits: %lu", (unsigned long)flockyou_stat()); break;
        case 4: snprintf(buf, sizeof(buf), "Packets: %lu", (unsigned long)pcap_stat()); break;
        case 5: snprintf(buf, sizeof(buf), "Drones: %d", skyspy_stat()); break;
        case 6: snprintf(buf, sizeof(buf), "Adverts: %lu", (unsigned long)blesniff_stat()); break;
        default: snprintf(buf, sizeof(buf), "Select via web"); break;  // selector
    }
    g_disp.print(buf);
    return false;
}

void oled_show(int mode) {
    if (!g_ok) return;
    g_disp.clearDisplay();
    g_disp.setTextColor(SSD1306_WHITE);

    // Header
    g_disp.setTextSize(2);
    g_disp.setCursor(0, 0);
    g_disp.print("OUI-SPY");
    g_disp.drawFastHLine(0, 18, OLED_W, SSD1306_WHITE);

    g_disp.setTextSize(1);

    // Row 1: mode name (left) + uptime m:ss (right-aligned)
    unsigned long secs = millis() / 1000UL;
    char up[12];
    snprintf(up, sizeof(up), "%lu:%02lu", secs / 60UL, secs % 60UL);
    g_disp.setCursor(0, 22);
    g_disp.print(mode_name(mode));
    int16_t bx, by; uint16_t bw, bh;
    g_disp.getTextBounds(up, 0, 0, &bx, &by, &bw, &bh);
    g_disp.setCursor(OLED_W - bw, 22);
    g_disp.print(up);

    // Row 2: live headline stat (the reason the OLED is worth having here).
    g_disp.setCursor(0, 34);
    int rssi = -100;
    bool fox = draw_stat(mode, &rssi);

    if (fox) {
        // Foxhunter proximity bar — the visual stand-in for the missing buzzer.
        // Map RSSI -100..-30 dBm onto 0..100% of the bar width.
        int pct = rssi <= -100 ? 0 : (rssi >= -30 ? 100 : (rssi + 100) * 100 / 70);
        g_disp.drawRect(0, 46, OLED_W, 12, SSD1306_WHITE);
        int fillw = (OLED_W - 4) * pct / 100;
        if (fillw > 0) g_disp.fillRect(2, 48, fillw, 8, SSD1306_WHITE);
    } else {
        // Row 3+4: connection. Live AP state stays correct if the SSID was
        // reconfigured; no-AP modes (Flock-You, Sky Spy) report USB.
        String ssid = WiFi.softAPSSID();
        g_disp.setCursor(0, 46);
        if (ssid.length() > 0) {
            g_disp.print(ssid);
            g_disp.setCursor(0, 56);
            g_disp.print(WiFi.softAPIP().toString());
        } else {
            g_disp.print("No AP - USB-CDC");
            g_disp.setCursor(0, 56);
            g_disp.print("serial output");
        }
    }

    g_disp.display();
}

void oled_tick(int mode) {
    if (!g_ok) return;
    static unsigned long last = 0;
    unsigned long now = millis();
    if (now - last < 1000UL) return;
    last = now;
    oled_show(mode);
}
