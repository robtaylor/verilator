# CXXRTL Server POC - Summary

## What We've Built

This proof-of-concept demonstrates the core architecture for implementing the CXXRTL debug server protocol in Verilator. It includes:

### 1. Runtime Library Components

**`verilated_cxxrtl_server.h/cpp`** - Located in `include/`
- `VlCxxrtlJson` class: Minimal JSON parser/serializer
- `VlCxxrtlServer` class: TCP socket server with protocol handling
- Thread-safe server that runs in background thread
- Protocol message handlers for basic commands

**Key Features:**
- ✅ TCP socket server (IPv4, port binding)
- ✅ JSON message framing (null-terminated)
- ✅ Protocol greeting exchange
- ✅ Basic command routing
- ✅ Runs in separate thread
- ✅ Cross-platform (Unix + Windows support)

### 2. Protocol Implementation

**Implemented Commands:**
- ✅ `greeting` - Capability negotiation
- ✅ `list_scopes` - Return design hierarchy
- ✅ `list_items` - List signals in a scope
- ✅ `get_simulation_status` - Get current sim time/status
- ⚠️  `reference_items` - Stub only
- ❌ `query_interval` - Not implemented
- ❌ `run_simulation` - Not implemented
- ❌ `pause_simulation` - Not implemented

### 3. Test Infrastructure

**Files:**
- `counter.v` - Simple RTL design (8-bit counter)
- `sim_main.cpp` - Testbench with integrated server
- `test_client.py` - Python client for protocol testing
- `Makefile` - Build automation

## What's Working

1. **Socket Communication** - Server accepts connections and exchanges JSON messages
2. **Protocol Compliance** - Greeting exchange follows CXXRTL spec v0.14.0
3. **Command Handling** - Basic commands parse and respond correctly
4. **Manual Integration** - Can be added to any Verilator model by including the library

## What's Missing (For Full Implementation)

### Critical (Required for MVP)

1. **Symbol Table Integration**
   - Currently uses dummy data
   - Need to walk `VerilatedScope` to build real scope hierarchy
   - Extract signal names, widths, types from Verilator's symbol table
   - Map hierarchical names to signal pointers

2. **Signal Value Access**
   - Need pointers to actual signal storage (CData, SData, IData, WData)
   - Implement value reading/writing through the protocol
   - Handle wide signals (>64 bits) correctly

3. **Code Generation Integration**
   - Add `--cxxrtl-server` command-line flag to `V3Options`
   - Modify `V3EmitCModel` to generate server initialization code
   - Modify `V3EmitCSyms` to emit enhanced metadata
   - Conditionally compile server support (no overhead when disabled)

### Important (For Usability)

4. **Replay Buffer System**
   - Capture state at each time step (inputs + flip-flops)
   - Support bidirectional navigation
   - Implement circular buffer with size limits
   - Differential storage (only changed signals)

5. **Query Interval Implementation**
   - Rewind simulation to requested time
   - Re-evaluate through interval
   - Capture requested signals
   - Encode as base64(u32) per protocol
   - Return waveform data

6. **Simulation Control**
   - `run_simulation` - Resume/start simulation
   - `pause_simulation` - Pause at current or specified time
   - Event generation (simulation_paused, simulation_finished)
   - Thread synchronization for control

### Nice-to-Have (For Polish)

7. **Thread Safety**
   - Mutex protection for multithreaded models
   - Coordinate with `VerilatedMutex`
   - Pause worker threads during queries
   - Deadlock prevention

8. **Better JSON Library**
   - Replace minimal parser with `json11` (from Yosys)
   - Proper error handling
   - Support full JSON spec

9. **Comprehensive Testing**
   - Unit tests for JSON parser
   - Protocol compliance tests
   - Multithreaded model tests
   - Large design stress tests
   - Comparison with VCD output

## Architecture Decisions Made

### 1. Separate Thread for Protocol
- **Pro**: Doesn't block simulation, responsive to clients
- **Con**: Requires thread synchronization
- **Alternative**: Event-driven in main loop (rejected - too invasive)

### 2. Optional Compilation (`--cxxrtl-server` flag)
- **Pro**: Zero overhead when not used
- **Con**: More complex code generation
- **Alternative**: Always included (rejected - unwanted dependency)

### 3. Manual Integration (POC)
- **Pro**: Quick to prototype, no Verilator changes
- **Con**: User must manually add server code
- **Next Step**: Auto-generate in full implementation

### 4. Minimal JSON Parser (POC)
- **Pro**: No external dependencies for POC
- **Con**: Limited, fragile, not spec-compliant
- **Next Step**: Use json11 in production

### 5. Dummy Scope Data (POC)
- **Pro**: Demonstrates protocol without symbol table complexity
- **Con**: Not connected to real design
- **Next Step**: Walk VerilatedScope in production

## Testing the POC

### Prerequisites
```bash
# Build Verilator first
cd /path/to/verilator
autoconf && ./configure && make -j$(nproc)
```

### Build and Run
```bash
cd test_regress/t/poc_cxxrtl

# Build
make build

# Run simulation (Terminal 1)
make run

# Connect client (Terminal 2)
python3 test_client.py
```

### Expected Output

**Simulation:**
```
CXXRTL debug server listening on port 12345
Simulation started with CXXRTL server on port 12345
Connect with: python3 test_client.py
Client connected
Received: {"type":"greeting","version":0}
...
```

**Client:**
```
Connecting to CXXRTL server on localhost:12345...
Connected!

=== Sending greeting ===
Sending: {"type": "greeting", "version": 0}
Received: {"type":"greeting","version":0,"commands":[...]}

Server capabilities:
  Commands: ['list_scopes', 'list_items', ...]

=== Listing scopes ===
Available scopes: ['top']

=== Listing items in 'top' ===
Items in 'top':
  clk: width=1, type=value
  rst: width=1, type=value

=== Test complete ===
```

## Implementation Roadmap

### Phase 1: Foundation (Completed in POC ✅)
- [x] Socket server with JSON protocol
- [x] Basic command handling
- [x] Test infrastructure
- [x] Architecture validation

### Phase 2: Integration (Next - 2-3 weeks)
- [ ] Add `--cxxrtl-server` flag to V3Options
- [ ] Integrate with symbol table (walk VerilatedScope)
- [ ] Generate server initialization in V3EmitCModel
- [ ] Connect to real signal pointers
- [ ] Test with real designs

### Phase 3: Replay & Queries (3-4 weeks)
- [ ] Implement replay buffer
- [ ] Add state capture hooks in eval()
- [ ] Implement query_interval
- [ ] Waveform extraction and encoding
- [ ] Test against VCD output

### Phase 4: Simulation Control (1-2 weeks)
- [ ] Implement run_simulation
- [ ] Implement pause_simulation
- [ ] Event generation
- [ ] Thread synchronization

### Phase 5: Polish (2-3 weeks)
- [ ] Replace JSON parser with json11
- [ ] Comprehensive test suite
- [ ] Performance optimization
- [ ] Documentation
- [ ] Examples

## Files Created

```
include/
  verilated_cxxrtl_server.h          (370 lines) - Server header
  verilated_cxxrtl_server.cpp        (450 lines) - Server implementation

test_regress/t/poc_cxxrtl/
  counter.v                          (11 lines)  - Test design
  sim_main.cpp                       (60 lines)  - Test harness
  test_client.py                     (90 lines)  - Test client
  Makefile                           (50 lines)  - Build script
  README.md                          (160 lines) - Usage docs
  POC_SUMMARY.md                     (this file)
```

**Total: ~1,200 lines of code/docs**

## Comparison with CXXRTL (Yosys)

| Feature | Yosys CXXRTL | Verilator POC | Notes |
|---------|-------------|---------------|-------|
| Socket Server | ❌ | ✅ | Yosys uses stdio, we use TCP |
| JSON Protocol | ❌ | ✅ | Yosys uses custom, we use CXXRTL spec |
| Simulation | ✅ | ✅ | Both generate C++ models |
| Debug Info | ✅ | ⚠️ | Yosys has debug_items, we need to build |
| Outline Objects | ✅ | ❌ | Yosys recomputes optimized signals |
| Value Access | ✅ | ⚠️ | Yosys has curr/next pointers |
| Multithreading | ❌ | ✅ | Verilator has MT, Yosys doesn't |
| Replay Buffer | ❌ | ❌ | Neither has it yet (protocol feature) |

## Key Insights

1. **Socket server works well** - TCP/JSON is a good transport choice
2. **Protocol is straightforward** - Message framing with null terminator is simple
3. **Symbol table access is key** - Most work will be in walking Verilator's internal structures
4. **Minimal overhead is achievable** - Conditional compilation keeps it zero-cost when unused
5. **Threading adds complexity** - But necessary for responsiveness

## Questions to Resolve

1. **How to handle wide signals in JSON?**
   - Current: base64(u32) encoding per protocol
   - Consider: Chunking for very wide signals?

2. **Replay buffer memory limit?**
   - Suggested: 100MB default, configurable
   - Trade-off: Memory vs. query range

3. **Thread synchronization granularity?**
   - Current plan: Lock entire eval()/commit() cycle
   - Alternative: Fine-grained locking per signal?

4. **SystemC support?**
   - POC is C++ only
   - SystemC models need special handling?

5. **Integration with existing tracing?**
   - Can we share code with VCD/FST tracers?
   - Reuse VerilatedTrace infrastructure?

## Conclusion

This POC successfully demonstrates that the CXXRTL debug server protocol can be implemented for Verilator with reasonable effort. The core architecture is sound, and the next phase (integration with Verilator's code generation) is well-defined.

**Estimated total implementation time: 8-12 weeks**
**Lines of code estimate: 3,000-4,000 (runtime + codegen)**

The proof-of-concept validates the approach and provides a solid foundation for the full implementation.
