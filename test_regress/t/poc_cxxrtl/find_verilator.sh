#!/bin/bash
# Helper script to find Verilator installation

# Try to find verilator_bin
find_verilator() {
    # 1. Check if VERILATOR_ROOT is already set and valid
    if [ -n "$VERILATOR_ROOT" ] && [ -f "$VERILATOR_ROOT/bin/verilator_bin" ]; then
        echo "$VERILATOR_ROOT"
        return 0
    fi

    # 2. Check relative path (in-tree)
    RELATIVE_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
    if [ -f "$RELATIVE_ROOT/bin/verilator_bin" ]; then
        echo "$RELATIVE_ROOT"
        return 0
    fi

    # 3. Check if verilator is in PATH
    VERILATOR_BIN=$(which verilator 2>/dev/null)
    if [ -n "$VERILATOR_BIN" ]; then
        # Follow symlinks
        VERILATOR_BIN=$(readlink -f "$VERILATOR_BIN" 2>/dev/null || realpath "$VERILATOR_BIN" 2>/dev/null || echo "$VERILATOR_BIN")
        # Get directory
        VERILATOR_DIR=$(dirname "$VERILATOR_BIN")
        # Go up one level to get root
        VERILATOR_ROOT=$(dirname "$VERILATOR_DIR")

        if [ -f "$VERILATOR_ROOT/bin/verilator_bin" ]; then
            echo "$VERILATOR_ROOT"
            return 0
        fi

        # Try system install location
        if [ -d "/usr/local/share/verilator" ]; then
            echo "/usr/local/share/verilator"
            return 0
        fi

        # Try /usr/share/verilator
        if [ -d "/usr/share/verilator" ]; then
            echo "/usr/share/verilator"
            return 0
        fi
    fi

    # Not found
    return 1
}

# Main
FOUND_ROOT=$(find_verilator)
if [ $? -eq 0 ]; then
    echo "Found Verilator at: $FOUND_ROOT"
    echo ""
    echo "To use this for building, run:"
    echo "  export VERILATOR_ROOT=\"$FOUND_ROOT\""
    echo "  make build"
    echo ""
    echo "Or build directly:"
    echo "  VERILATOR_ROOT=\"$FOUND_ROOT\" make build"
    exit 0
else
    echo "ERROR: Could not find Verilator installation" >&2
    echo "" >&2
    echo "Please either:" >&2
    echo "  1. Build Verilator in-tree (cd ../../.. && autoconf && ./configure && make)" >&2
    echo "  2. Install Verilator system-wide (sudo make install)" >&2
    echo "  3. Set VERILATOR_ROOT manually: export VERILATOR_ROOT=/path/to/verilator" >&2
    exit 1
fi
