#!/bin/zsh
# ============================================================================
# OUI SPY - Unified Firmware Sync & Build
#
# Usage:
#   ./sync.sh           Sync all repos + build
#   ./sync.sh 3         Sync only mode 3 (UniPwn) + build
#   ./sync.sh status    Just check what's changed (no build)
# ============================================================================

SCRIPT_DIR="${0:A:h}"
RAW_DIR="$SCRIPT_DIR/src/raw"
SRC_DIR="$SCRIPT_DIR/src"
PARENT_DIR="${SCRIPT_DIR:h}"
PIO="$HOME/.platformio/penv/bin/pio"

RED=$'\e[0;31m'; GREEN=$'\e[0;32m'; YELLOW=$'\e[1;33m'
BLUE=$'\e[0;34m'; CYAN=$'\e[0;36m'; BOLD=$'\e[1m'; NC=$'\e[0m'

repo_dir()  { case $1 in 1) echo "ouispy-detector";; 2) echo "ouispy-foxhunter";; 3) echo "Oui-Spy-UniPwn";; 4) echo "flock-you";; 5) echo "Sky-Spy";; esac }
mode_name() { case $1 in 1) echo "Detector";; 2) echo "Foxhunter";; 3) echo "UniPwn";; 4) echo "Flock-You";; 5) echo "Sky Spy";; esac }

sync_mode() {
    local m=$1
    local repo=$(repo_dir $m)
    local name=$(mode_name $m)
    local rp="$PARENT_DIR/$repo"

    if [[ ! -d "$rp" ]]; then
        echo "  ${RED}[SKIP]${NC} $repo not found"
        return 1
    fi

    # Git pull if repo
    if [[ -d "$rp/.git" ]]; then
        (cd "$rp" && git pull --quiet 2>/dev/null) || true
    fi

    echo -n "  ${BLUE}[$m]${NC} $name "

    case $m in
        1) cp "$rp/src/main.cpp" "$RAW_DIR/detector.cpp" ;;
        2) cp "$rp/src/main.cpp" "$RAW_DIR/foxhunter.cpp" ;;
        3)
            cp "$rp/src/config.h"              "$RAW_DIR/config.h"
            cp "$rp/src/unipwn-oui-spy.ino"    "$RAW_DIR/unipwn_main.cpp"
            cp "$rp/src/exploitation.ino"       "$RAW_DIR/unipwn_exploit.cpp"
            cp "$rp/src/hardware_feedback.ino"  "$RAW_DIR/unipwn_hardware.cpp"
            cp "$rp/src/web_interface.ino"      "$RAW_DIR/unipwn_web.cpp"
            ;;
        4) cp "$rp/src/main.cpp" "$RAW_DIR/flockyou.cpp" ;;
        5)
            cp "$rp/src/main.cpp"       "$RAW_DIR/skyspy.cpp"
            cp "$rp/src/opendroneid.h"  "$SRC_DIR/opendroneid.h"
            cp "$rp/src/opendroneid.c"  "$SRC_DIR/opendroneid.c"
            cp "$rp/src/odid_wifi.h"    "$SRC_DIR/odid_wifi.h"
            cp "$rp/src/wifi.c"         "$SRC_DIR/wifi.c"
            ;;
    esac
    echo "${GREEN}synced${NC}"
}

check_mode() {
    local m=$1
    local repo=$(repo_dir $m)
    local rp="$PARENT_DIR/$repo"
    local changed=0

    [[ ! -d "$rp" ]] && return

    case $m in
        1) diff -q "$rp/src/main.cpp" "$RAW_DIR/detector.cpp" &>/dev/null || changed=1 ;;
        2) diff -q "$rp/src/main.cpp" "$RAW_DIR/foxhunter.cpp" &>/dev/null || changed=1 ;;
        3) for p in "config.h:config.h" "unipwn-oui-spy.ino:unipwn_main.cpp" "exploitation.ino:unipwn_exploit.cpp" "hardware_feedback.ino:unipwn_hardware.cpp" "web_interface.ino:unipwn_web.cpp"; do
               diff -q "$rp/src/${p%%:*}" "$RAW_DIR/${p##*:}" &>/dev/null || changed=1
           done ;;
        4) diff -q "$rp/src/main.cpp" "$RAW_DIR/flockyou.cpp" &>/dev/null || changed=1 ;;
        5) diff -q "$rp/src/main.cpp" "$RAW_DIR/skyspy.cpp" &>/dev/null || changed=1
           for f in opendroneid.h opendroneid.c odid_wifi.h wifi.c; do
               diff -q "$rp/src/$f" "$SRC_DIR/$f" &>/dev/null || changed=1
           done ;;
    esac

    local name=$(mode_name $m)
    if [[ $changed -eq 1 ]]; then
        echo "  ${BLUE}[$m]${NC} $name ${YELLOW}needs sync${NC}"
    else
        echo "  ${BLUE}[$m]${NC} $name ${GREEN}up to date${NC}"
    fi
}

# ============================================================================
echo ""
echo "${CYAN}${BOLD}  OUI SPY - Sync & Build${NC}"
echo ""

case "${1:-sync}" in
    status)
        echo "${YELLOW}Checking...${NC}"
        echo ""
        for m in 1 2 3 4 5; do check_mode $m; done
        ;;
    [1-5])
        echo "${GREEN}Syncing mode $1...${NC}"
        echo ""
        sync_mode $1
        echo ""
        echo "${CYAN}Building...${NC}"
        cd "$SCRIPT_DIR" && $PIO run 2>&1
        ;;
    *)
        echo "${GREEN}Syncing all firmwares...${NC}"
        echo ""
        for m in 1 2 3 4 5; do sync_mode $m; done
        echo ""
        echo "${CYAN}Building...${NC}"
        cd "$SCRIPT_DIR" && $PIO run 2>&1
        ;;
esac

echo ""
