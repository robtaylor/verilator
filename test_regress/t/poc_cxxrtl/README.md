# CXXRTL Server Proof-of-Concept

This directory contains a proof-of-concept implementation of the CXXRTL debug server protocol for Verilator.

## What This POC Demonstrates

- TCP socket server that accepts remote connections
- JSON-based protocol message exchange
- Basic commands: greeting, list_scopes, list_items, get_simulation_status
- Integration with a Verilator model (simple counter)
- Python test client

## Files

- `counter.v` - Simple counter RTL design
- `sim_main.cpp` - Simulation harness with integrated CXXRTL server
- `test_client.py` - Python client to test the protocol
- `Makefile` - Build and run scripts

## Prerequisites

1. **Verilator** - Built or installed

   **Option A - Out-of-tree build:**
   ```bash
   export VERILATOR_ROOT=/path/to/your/verilator/build
   ```

   **Option B - System install:**
   ```bash
   export VERILATOR_ROOT=/usr/local/share/verilator
   ```

   **Option C - Build in-tree:**
   ```bash
   cd /path/to/verilator
   autoconf && ./configure && make -j$(nproc)
   # Will auto-detect from relative path
   ```

2. **Python 3** - For the test client

## Building and Running

### Build the simulation:
```bash
# If VERILATOR_ROOT is exported:
make build

# Or specify inline:
VERILATOR_ROOT=/path/to/verilator make build
```

### Run interactively:
```bash
# Terminal 1: Start simulation
make run

# Terminal 2: Connect with test client
python3 test_client.py
```

### Automated test:
```bash
make test
```

## Protocol Messages

### Greeting Exchange
Client sends:
```json
{"type": "greeting", "version": 0}
```

Server responds:
```json
{
  "type": "greeting",
  "version": 0,
  "commands": ["list_scopes", "list_items", ...],
  "events": [],
  "features": {"item_values_encoding": ["base64(u32)"]}
}
```

### List Scopes
```json
{"type": "command", "command": "list_scopes"}
```

Response:
```json
{"type": "response", "scopes": ["top"]}
```

### List Items
```json
{"type": "command", "command": "list_items", "scope": "top"}
```

Response:
```json
{
  "type": "response",
  "items": [
    {"name": "clk", "type": "value", "width": 1},
    {"name": "rst", "type": "value", "width": 1}
  ]
}
```

## Current Limitations (POC)

1. **Dummy scope data** - Not yet integrated with Verilator's actual symbol table
2. **No signal value access** - Can list items but not read their values
3. **No waveform queries** - query_interval not implemented
4. **No replay buffer** - No simulation rewind capability
5. **No simulation control** - Can't actually run/pause simulation remotely
6. **Minimal JSON parser** - Simple parser that assumes well-formed input
7. **Single client** - Only one client connection at a time
8. **No thread safety** - Protocol access not synchronized with eval()

## Next Steps

To turn this POC into a production feature:

1. **Symbol table integration** - Walk Verilator's VerilatedScope to build real scope hierarchy
2. **Signal value access** - Add pointers to actual signal storage
3. **Replay buffer** - Implement state capture and rewind
4. **Query implementation** - Support query_interval with waveform extraction
5. **Simulation control** - Implement run_simulation/pause_simulation
6. **Thread safety** - Add mutexes for multithreaded models
7. **Code generation** - Integrate into Verilator's code generator (--cxxrtl-server flag)
8. **Proper JSON** - Use json11 or similar library
9. **Comprehensive tests** - Test suite for all commands and edge cases

## References

- CXXRTL Protocol: https://cxxrtl.org/protocol.html
- Yosys CXXRTL backend: `../yosys/backends/cxxrtl/`
