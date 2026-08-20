/*
 * OUI SPY - on-board OLED status display (Heltec WiFi LoRa 32 V4)
 *
 * The Heltec V4 has an on-board 128x64 SSD1306 OLED (I2C SDA=17/SCL=18, RST=21,
 * powered via Vext=36). This module drives a status screen — mode name, AP
 * SSID/IP or USB, uptime, and a live per-mode stat — giving the buzzer-less
 * Heltec local feedback without the web UI. Driven from main.cpp; the per-mode
 * stat is read through the small getters declared in modes.h.
 */
#ifndef OUISPY_OLED_STATUS_H
#define OUISPY_OLED_STATUS_H

// Power up + initialise the OLED. Call once from setup(). Logs the SSD1306 init
// result to Serial (before any mode takes over the port).
void oled_begin();
// Draw the status screen for `mode` immediately.
void oled_show(int mode);
// Throttled refresh (~1 Hz) — call every loop() to keep the stat/uptime fresh.
void oled_tick(int mode);

#endif // OUISPY_OLED_STATUS_H
