# CXXRTL Server POC - Quick Start

Get the proof-of-concept running in 5 minutes.

## Step 1: Find Verilator

### Quick Auto-Detect
```bash
cd test_regress/t/poc_cxxrtl
./find_verilator.sh
```

This will find your Verilator installation and tell you what to do.

### Manual Options

**Option A: Use an existing Verilator build**
```bash
export VERILATOR_ROOT=/path/to/your/verilator/build
```

**Option B: Use system-installed Verilator**
```bash
export VERILATOR_ROOT=/usr/local/share/verilator
# Or wherever 'which verilator' points
```

**Option C: Build Verilator in-tree**
```bash
cd /path/to/verilator
autoconf
./configure
make -j$(nproc)
# VERILATOR_ROOT will auto-detect
```

## Step 2: Build the POC

```bash
cd test_regress/t/poc_cxxrtl

# If you set VERILATOR_ROOT above:
make build

# Or specify it inline:
VERILATOR_ROOT=/path/to/verilator make build
```

If you get an error about `verilator_bin` not found, check your VERILATOR_ROOT path.

## Step 3: Run It!

### Option A: Automated Test
```bash
make test

# Or with out-of-tree build:
VERILATOR_ROOT=/path/to/verilator make test
```

This runs the simulation in the background and connects a test client.

### Option B: Manual (Two Terminals)

Terminal 1 - Start simulation:
```bash
make run

# Or with out-of-tree build:
VERILATOR_ROOT=/path/to/verilator make run
```

You should see:
```
CXXRTL debug server listening on port 12345
Simulation started with CXXRTL server on port 12345
```

Terminal 2 - Connect client:
```bash
python3 test_client.py
```

You should see protocol messages and responses.

## What You'll See

The test client will:
1. Connect to the server
2. Exchange greeting with capability negotiation
3. List available scopes (just "top" in POC)
4. List items in the "top" scope (clk, rst)
5. Query simulation status (time, running state)

## Troubleshooting

**Port already in use:**
```bash
# Find what's using port 12345
lsof -i :12345

# Kill it or change port in sim_main.cpp
```

**Connection refused:**
- Make sure simulation is running
- Check firewall isn't blocking localhost:12345

**Build errors:**
- Ensure Verilator is fully built
- Check that you're in the `poc_cxxrtl` directory

## Next Steps

1. **Read POC_SUMMARY.md** - Understand what's implemented and what's not
2. **Read README.md** - Detailed usage and protocol messages
3. **Modify counter.v** - Try adding more signals and see them appear
4. **Extend the protocol** - Add your own commands to the server

## Customization

### Change the port:
Edit `sim_main.cpp`:
```cpp
server.start(12345, top.get(), nullptr);  // Change 12345 to your port
```

### Add more signals:
Edit `counter.v` to add new signals, rebuild, and they'll show in `list_items`.

### Try a different design:
Replace `counter.v` with your own Verilog, update the Makefile, and rebuild.

## Understanding the Code

**Start here:**
1. `test_client.py` - See how to interact with the protocol
2. `sim_main.cpp` - See how to integrate the server
3. `verilated_cxxrtl_server.cpp` - See the protocol implementation

**Key concepts:**
- JSON messages with null terminators
- Command/response pattern
- Scope hierarchy (will be real in full version)
- Item metadata (name, type, width)

## Have Fun!

This is a proof-of-concept - it's meant to be explored and modified. Try breaking it, extending it, and understanding how it works. That's the best way to prepare for the full implementation.

Questions? See POC_SUMMARY.md for architecture details and implementation roadmap.
