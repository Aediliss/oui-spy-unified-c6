/*
 * OUI SPY - NimBLE 1.4.x -> 2.x compatibility shim
 * =====================================================================
 * The XIAO ESP32-C6's Bluetooth controller is only supported by
 * NimBLE-Arduino 2.x, whose scan API differs from the 1.4.x API the four
 * BLE modes (Detector, Foxhunter, Sky Spy, BLE Sniff) are written in.
 * The S3 build keeps NimBLE 1.4.x; this header lets the SAME mode sources
 * compile against both, so neither target needs #ifdefs sprinkled through
 * the mode logic.
 *
 * Include it in each BLE mode wrapper AFTER <NimBLEDevice.h> and BEFORE
 * the `#include "raw/<mode>.cpp"` line.
 *
 * What changed in NimBLE 2.x, and how it's bridged:
 *   - Callback base class NimBLEAdvertisedDeviceCallbacks was renamed to
 *     NimBLEScanCallbacks, and onResult() now takes a const pointer.
 *     -> On the C6 we re-declare NimBLEAdvertisedDeviceCallbacks as a thin
 *        adapter over NimBLEScanCallbacks that forwards the const callback
 *        to the old non-const onResult() the modes implement.
 *   - NimBLEScan::setAdvertisedDeviceCallbacks() -> setScanCallbacks()
 *     (same trailing wantDuplicates arg). -> macro rename on the C6.
 *   - NimBLEScan::start() takes milliseconds (was seconds) and returns bool
 *     (was NimBLEScanResults); the old scan-complete callback arg is gone.
 *     -> OUISPY_BLE_START() keeps the call sites' "seconds" intent.
 *   - NimBLEAdvertisedDevice::getPayload() returns a std::vector (was a raw
 *     pointer), getPayloadLength() is gone, and NimBLEAddress::getNative()
 *     became getVal().
 *     -> OUISPY_BLE_PAYLOAD / _PAYLOAD_LEN / _ADDR_BYTES bridge those.
 *
 * The OUISPY_BLE_* helpers are defined for BOTH versions so the (edited)
 * call sites are identical on S3 and C6.
 */
#ifndef OUISPY_NIMBLE_COMPAT_C6_H
#define OUISPY_NIMBLE_COMPAT_C6_H

#include "compat_esp32c6.h"   /* defines OUISPY_TARGET_C6 on a C6 build */

#ifdef __cplusplus
#include <NimBLEDevice.h>

#ifdef OUISPY_TARGET_C6
/* ---- NimBLE 2.x (ESP32-C6) ------------------------------------------- */

/* Adapter so `class X : public NimBLEAdvertisedDeviceCallbacks { void
 * onResult(NimBLEAdvertisedDevice*) }` still compiles. The modes override
 * the old non-const onResult(); we override the 2.x const one and forward.
 * (Virtual dispatch, not name lookup, so the forward always reaches the
 * mode's implementation.) */
class NimBLEAdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
  public:
    virtual void onResult(NimBLEAdvertisedDevice* advertisedDevice) = 0;
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        onResult(const_cast<NimBLEAdvertisedDevice*>(advertisedDevice));
    }
};

/* setAdvertisedDeviceCallbacks(cb[, wantDuplicates]) -> setScanCallbacks */
#define setAdvertisedDeviceCallbacks setScanCallbacks

/* NimBLEDevice::getInitialized() was renamed to isInitialized() in 2.x */
#define getInitialized isInitialized

/* start(seconds, scanEndCB, isContinue) -> start(ms, isContinue) */
#define OUISPY_BLE_START(scan, sec, cont) \
        (scan)->start((uint32_t)(sec) * 1000u, (cont))

/* getPayload() -> vector; getPayloadLength() gone; getNative() -> getVal() */
#define OUISPY_BLE_PAYLOAD(dev)      ((dev)->getPayload().data())
#define OUISPY_BLE_PAYLOAD_LEN(dev)  ((dev)->getPayload().size())
#define OUISPY_BLE_ADDR_BYTES(addr)  ((addr).getVal())

#else
/* ---- NimBLE 1.4.x (ESP32-S3) — original API ---------------------------*/
#define OUISPY_BLE_START(scan, sec, cont) \
        (scan)->start((sec), nullptr, (cont))
#define OUISPY_BLE_PAYLOAD(dev)      ((dev)->getPayload())
#define OUISPY_BLE_PAYLOAD_LEN(dev)  ((dev)->getPayloadLength())
#define OUISPY_BLE_ADDR_BYTES(addr)  ((addr).getNative())
#endif /* OUISPY_TARGET_C6 */

#endif /* __cplusplus */
#endif /* OUISPY_NIMBLE_COMPAT_C6_H */
