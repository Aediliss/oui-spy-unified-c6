/*
 * OUI SPY - Mode lifecycle interface
 *
 * Every detection mode is described by a ModeDef: identity/metadata plus the
 * three lifecycle hooks (setup / loop / stop). The ModeManager owns exclusive
 * activation: only one mode runs at a time, and switching from A to B always
 * goes A.stop() -> release radios -> B.setup(). This is the engine the on-watch
 * pager UI drives; on the XIAO it replaces the reboot-per-mode selector.
 */
#ifndef MODE_IFACE_H
#define MODE_IFACE_H

#include <stdint.h>

typedef void (*mode_fn)();

struct ModeDef {
    const char* id;    // stable short id, e.g. "detector"
    const char* name;  // display name, e.g. "DETECTOR"
    const char* desc;  // one-line description for the page
    mode_fn setup;     // acquire hardware + start scanning/serving
    mode_fn loop;      // called every tick while this mode is active
    mode_fn stop;      // release everything setup() acquired (may be nullptr)
};

namespace ModeManager {

// Call once from Arduino setup(). Starts in the IDLE state (no mode active).
void begin();

// Call every Arduino loop(). Runs the active mode's loop(), or idles.
void tick();

// Registry access (fixed order == pager order).
int count();
const ModeDef& at(int i);

// -1 when no mode is active (IDLE), otherwise the active registry index.
int activeIndex();

// Exclusive activation: stops the current mode (if any), releases the radios,
// then runs the target mode's setup(). No-op if i is already active.
void activate(int i);

// Stop the active mode and return to IDLE. No-op when already idle.
void deactivate();

// Generic radio/peripheral teardown run between modes so a fresh setup()
// starts from a clean WiFi/BLE state. Modes' own stop() handle mode-specific
// resources (web server, buffers, timers); this handles the shared radio.
void releaseRadios();

} // namespace ModeManager

#endif // MODE_IFACE_H
