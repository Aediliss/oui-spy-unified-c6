/*
 * OUI SPY - ESP32-C6 compatibility shim
 * =====================================================================
 * The original firmware targets the Seeed XIAO ESP32-S3 built against
 * Arduino-ESP32 core 2.x. Porting to the ESP32-C6 forces a jump to
 * Arduino-ESP32 core 3.x (the first core with C6 support), and the C6 is
 * a single-core RISC-V part with no PSRAM. This header papers over the
 * three source-level breakages that jump introduces, plus the pin map,
 * WITHOUT touching the vendored per-mode source in src/raw/.
 *
 *   1. LEDC API break. Core 3.x removed the channel-based buzzer API
 *      (ledcSetup / ledcAttachPin) and made ledcWrite / ledcWriteTone
 *      take a *pin* instead of a channel. Every mode drives a single
 *      buzzer on LEDC channel 0, so we re-implement the old channel-0
 *      calls on top of the new pin-based API.
 *
 *   2. Single core. xTaskCreatePinnedToCore(..., 1) asserts on the C6,
 *      which only has core 0. We clamp any out-of-range core id to
 *      tskNO_AFFINITY.
 *
 *   3. Pin map. The XIAO ESP32-C6 has a different pinout than the S3.
 *      We publish C6 defaults here; the mode sources pick them up
 *      because their own #defines are now #ifndef-guarded.
 *
 * This whole file is a no-op unless we're building for the C6, so the
 * ESP32-S3 build is completely unchanged. Include it from each mode
 * wrapper AFTER <Arduino.h> and the FreeRTOS headers, and BEFORE the
 * `#include "raw/<mode>.cpp"` line.
 */
#ifndef OUISPY_COMPAT_ESP32C6_H
#define OUISPY_COMPAT_ESP32C6_H

/* Detect the C6 target. ARDUINO_XIAO_ESP32C6 is defined by the board;
 * CONFIG_IDF_TARGET_ESP32C6 covers a generic C6 board (e.g. devkitc). */
#if defined(ARDUINO_XIAO_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32C6)
#define OUISPY_TARGET_C6 1
#endif

#ifdef OUISPY_TARGET_C6
#ifdef __cplusplus

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* -------------------------------------------------------------------------
 * 3. Pin map — Seeed XIAO ESP32-C6
 *
 *   Header pads:  D0=0  D1=1  D2=2  D3=21  D4=22(SDA)  D5=23(SCL)
 *                 D6=16(TX) D7=17(RX) D8=19(SCK) D9=20(MISO) D10=18(MOSI)
 *   On-board LED: GPIO15, active-low (LOW = on) — same inverted logic the
 *                 modes already assume for the S3's GPIO21 LED.
 *   BOOT button:  GPIO9, active-low with pull-up (LOW = pressed) — same
 *                 logic the selector already uses for the S3's GPIO0.
 *
 * The modes' own pin #defines are #ifndef-guarded, so defining them here
 * (before the raw source is included) wins without editing that source.
 * Wire an external buzzer to D2 and the optional Detector NeoPixel to D1.
 * ---------------------------------------------------------------------- */
#ifndef LED_PIN
#define LED_PIN 15          /* on-board user LED (active-low)            */
#endif
#ifndef BOOT_BUTTON_PIN
#define BOOT_BUTTON_PIN 9   /* BOOT button (active-low, pull-up)        */
#endif
#ifndef BUZZER_PIN
#define BUZZER_PIN 2        /* external piezo buzzer on D2              */
#endif
#ifndef PCAP_BUZZER_PIN
#define PCAP_BUZZER_PIN 2   /* PCAP mode buzzer (IDF LEDC) on D2         */
#endif
#ifndef BLESNIFF_BUZZER_PIN
#define BLESNIFF_BUZZER_PIN 2 /* BLE Sniff mode buzzer (IDF LEDC) on D2 */
#endif
#ifndef NEOPIXEL_PIN
#define NEOPIXEL_PIN 1      /* optional Detector NeoPixel on D1         */
#endif
#ifndef PCAP_LED_PIN
#define PCAP_LED_PIN 15     /* PCAP mode status LED = on-board LED       */
#endif
#ifndef BLESNIFF_LED_PIN
#define BLESNIFF_LED_PIN 15 /* BLE Sniff mode status LED = on-board LED  */
#endif
#ifndef MIRROR_TX_PIN
#define MIRROR_TX_PIN 20    /* Flock-You Serial1 debug mirror on D9      */
#endif

/* -------------------------------------------------------------------------
 * 1. LEDC 2.x -> 3.x buzzer shim
 *
 * These helpers are defined BEFORE the compatibility macros below, so
 * their bodies bind to the real core-3.x pin-based LEDC functions. Only
 * LEDC channel 0 is ever used in this firmware, and always for the single
 * buzzer, so we track exactly one attached pin.
 * ---------------------------------------------------------------------- */
static inline int&      _ouispy_ledc_pin()  { static int v = -1;   return v; }
static inline uint32_t& _ouispy_ledc_freq() { static uint32_t v = 2000; return v; }
static inline uint8_t&  _ouispy_ledc_res()  { static uint8_t v = 8;  return v; }

/* Old: ledcSetup(channel, freq, resolution_bits). If the pin is already
 * attached (mid-melody frequency change), retune it; otherwise just
 * remember the settings for the next attach. */
static inline void ouispy_ledc_setup(uint32_t freq, uint8_t res) {
    _ouispy_ledc_freq() = freq;
    _ouispy_ledc_res()  = res;
    int pin = _ouispy_ledc_pin();
    if (pin >= 0) ledcChangeFrequency((uint8_t)pin, freq, res);
}

/* Old: ledcAttachPin(pin, channel). Attach once at the remembered
 * frequency/resolution. */
static inline void ouispy_ledc_attach(int pin) {
    if (_ouispy_ledc_pin() != pin) {
        ledcAttach((uint8_t)pin, _ouispy_ledc_freq(), _ouispy_ledc_res());
        _ouispy_ledc_pin() = pin;
    }
}

/* Old: ledcWrite(channel, duty). */
static inline void ouispy_ledc_write(uint32_t duty) {
    int pin = _ouispy_ledc_pin();
    if (pin >= 0) ledcWrite((uint8_t)pin, duty);
}

/* Old: ledcWriteTone(channel, freq). */
static inline void ouispy_ledc_tone(uint32_t freq) {
    int pin = _ouispy_ledc_pin();
    if (pin >= 0) ledcWriteTone((uint8_t)pin, freq);
}

/* -------------------------------------------------------------------------
 * 2. Single-core task pinning
 *
 * Defined before the macro so the body binds to the real function. Any
 * core id the C6 doesn't have (i.e. core 1) becomes tskNO_AFFINITY.
 * ---------------------------------------------------------------------- */
static inline BaseType_t ouispy_pin_core(BaseType_t core) {
    return (core >= 0 && core < portNUM_PROCESSORS) ? core : tskNO_AFFINITY;
}

/* -------------------------------------------------------------------------
 * Compatibility macros. Text-substituted at the call sites in the raw
 * mode sources that are #included after this header. The old function
 * names no longer exist in core 3.x (ledcSetup/ledcAttachPin) or take a
 * pin (ledcWrite/ledcWriteTone), so remapping channel-0 calls is safe.
 * The self-referential xTaskCreatePinnedToCore macro is not re-expanded
 * (standard preprocessor "blue paint" rule) and this header is included
 * only after <freertos/task.h>, so the real declaration is untouched.
 * ---------------------------------------------------------------------- */
#define ledcSetup(ch, freq, res)   ouispy_ledc_setup((freq), (res))
#define ledcAttachPin(pin, ch)     ouispy_ledc_attach((pin))
#define ledcWrite(ch, duty)        ouispy_ledc_write((duty))
#define ledcWriteTone(ch, freq)    ouispy_ledc_tone((freq))

#define xTaskCreatePinnedToCore(fn, name, stack, param, prio, handle, core) \
        xTaskCreatePinnedToCore((fn), (name), (stack), (param), (prio), (handle), \
                                ouispy_pin_core((core)))

#endif /* __cplusplus */
#endif /* OUISPY_TARGET_C6 */
#endif /* OUISPY_COMPAT_ESP32C6_H */
