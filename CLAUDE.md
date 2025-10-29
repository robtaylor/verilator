# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Verilator is the fastest Verilog/SystemVerilog simulator. It compiles Verilog/SystemVerilog code into multithreaded C++ or SystemC, performs lint checks, and creates XML for custom tools. Licensed under LGPL-3.0 or Artistic-2.0.

## Build System

Verilator supports two build systems:

### Autoconf/Make (Primary)
```bash
# Initial configuration
autoconf
./configure
make -j$(nproc)

# After configuration is already done
make -j$(nproc)

# Install
make install

# Clean builds
make clean        # Delete build artifacts
make distclean    # Delete all generated files including configuration
```

### CMake (Experimental on Linux/macOS)
```bash
# Configure and build
cmake -S . -B build
cmake --build build -j$(nproc)

# Install
cmake --install build

# Build with debug and release
cmake -S . -B build -DDEBUG_AND_RELEASE_AND_COVERAGE=ON
```

**Note**: CMake support on Linux/macOS is experimental; autoconf/make is the primary build system.

## Testing

### Running Tests

Tests are located in `test_regress/` and use Python driver scripts:

```bash
# Run all tests (default scenarios: --vlt --vltmt --dist)
cd test_regress && make test

# Run with specific scenarios
cd test_regress && python3 driver.py --vlt            # Verilator single-threaded
cd test_regress && python3 driver.py --vltmt          # Verilator multi-threaded
cd test_regress && python3 driver.py --vcs            # Synopsys VCS
cd test_regress && python3 driver.py --nc             # Cadence NC
cd test_regress && python3 driver.py --xsim           # Xilinx Xsim

# Run tests in parallel
cd test_regress && python3 driver.py -j 0             # Auto-detect cores

# Run specific test
cd test_regress && python3 driver.py t/t_<testname>.py
```

### Test File Structure

- Test files: `test_regress/t/t_*.py` (Python test scripts)
- Test uses `vltest_bootstrap` module and defines scenarios with `test.scenarios()`
- Tests typically call `test.compile()` then `test.execute()` then `test.passes()`
- Tests with `_bad` suffix are expected to fail

### Writing New Tests

Test format (see `test_regress/t/t_a1_first_cc.py` for example):
```python
#!/usr/bin/env python3
import vltest_bootstrap

test.scenarios('vlt')  # Scenarios: vlt, vltmt, dist, vcs, etc.
test.compile(verilator_flags2=["--trace-vcd"])
test.execute()
test.passes()
```

## Code Architecture

### High-Level Flow

Verilator processes files through multiple transformation passes (see `src/Verilator.cpp::process()`):

1. **Parsing**: Flex/Bison parse Verilog → Abstract Syntax Tree (AST)
2. **Linking**: Link cells, read additional files, resolve references
3. **Elaboration**: Resolve parameters, elaborate design
4. **Optimization**: Coverage, assertions, X elimination, inlining, constant propagation
5. **Pseudo-flattening**: Create scope references (VarScope) for each module instance
6. **Flattened optimization**: Inlining, loop unrolling, lifetime analysis, gate simplifications
7. **Ordering**: Create single eval function with optimal execution order
8. **Un-flattening**: Share code between module instances, localize variables
9. **Code generation**: Write C++ modules

### Key Source Components

- `src/V3*.cpp/h`: Compiler passes (each pass transforms the AST)
  - `V3Active.cpp`: Active logic handling
  - `V3Assert.cpp`: Assertion insertion
  - `V3Order.cpp`: Statement ordering for optimal evaluation
  - `V3Sched.cpp`: Scheduling logic (static scheduler, timing features)
  - `V3Timing.cpp`: Timing control transformations (delays, event controls)
  - `V3Split*.cpp`: Various code splitting passes
  - `V3EmitC*.cpp`: C++ code emission
- `src/V3Ast*.cpp/h`: AST node definitions
- `src/V3Graph*.cpp/h`: Graph algorithms for ordering and partitioning
- `src/astgen`: Script to generate repetitive AST-related code
- `include/`: Runtime library headers (verilated.h, verilated_vcd_c.h, etc.)

### AST Node Hierarchy

- All AST nodes derive from `AstNode`
- Non-final subclasses must be abstract and named `AstNode*` (e.g., `AstNodeFTask`)
- Each node has up to 4 children via `op1p()` through `op4p()` methods
- Nodes have `next()` and `back()` for sequential statements
- `AstNetlist` is the root of the AST
- Notable hierarchies:
  - `AstNodeDType`: All data type nodes
  - `AstNodeExpr`: All expression nodes

### Passes and Visitors

- Passes are implemented as `VNVisitor` subclasses
- Each visitor implements `visit()` methods for AST node types
- Convention: use `nodep` variable for the current AST node being processed

## Coding Conventions

### C++ Style

- **C++ version**: C++14 required, C++17/C++20 compatible
- **Naming**: mixedCapsSymbols (not underlined_symbols)
- **Pointers**: Use "p" suffix (e.g., `nodep`, `varscope p`)
- **Indentation**: 4 spaces per level, NO tabs
- **Spacing**:
  - 2 spaces between code and comment
  - 1 space after if/for/switch/while keywords
  - No space between function name and open paren
  - No space before semicolons
- **Comments**: Comment every member variable
- **Auto-formatting**: Run `make format` (uses clang-format 18)
- **Line limit**: 99 characters

### Python Style

- **Formatting**: Use yapf (`.style.yapf` config)
- **Auto-formatting**: `make format` also formats Python

### Verilog Style (tests)

- **Indentation**: 2 spaces per level, NO tabs
- **begin/end**: Place `begin` on same line as if/else, `end` on separate line

### Commit Messages

Use these patterns (include GitHub issue/PR numbers):

- `Add <feature> (#1234)` - New features
- `Fix <item> (#1234)` - Bug fixes
- `Improve <item> (#1234)` - Enhancements
- `Optimize <item> (#1234)` - Performance improvements
- `Support <feature> (#1234)` - IEEE-specified features
- `Tests: <Add/Improve/Fix> (#1234)` - Test-only changes
- `Internals: <Add/Improve/Fix> (#1234)` - Developer-only changes (code cleanups)
- `CI: <Add/Improve/Fix> (#1234)` - GitHub Actions changes
- `Commentary` - Trivial documentation changes

Add "No functional change." or "No functional change expected." when appropriate.

## Development Workflow

### Code Formatting

```bash
# Auto-format all C++ and Python code
make format
```

Uses clang-format 18 for C/C++ and yapf for Python.

### Adding a New Pass

1. Copy `.cpp` and `.h` files from an existing similar pass
2. Add call to new pass in `src/Verilator.cpp::process()`
3. Update `src/Makefile_obj.in` to include new files
4. Follow naming: `V3PassName.cpp` and `V3PassName.h`
5. Implement as `VNVisitor` subclass

### AST Code Generation

The `astgen` script generates repetitive AST code:

- Add `@astgen` directives in comments within AstNode subclass definitions
- Include `ASTGEN_MEMBERS_AstFoo;` in class body
- Common directives: `@astgen op1`, `@astgen op2`, `@astgen op3`, `@astgen op4`
- Run build process to regenerate code

### Multithreading Architecture

Verilator uses static scheduling for multithreaded models:

- **Partitioning**: Graph-based partitioning using edge contraction (Sarkar's algorithm)
- **Macro-tasks**: Combined atomic tasks that execute on one thread without synchronization
- **Cost estimation**: `InstrCountVisitor` estimates execution costs
- **Spatial locality**: Variables ordered by footprint (TSP approximation) for cache performance
- **Profiling**: Use `--prof-exec` and `verilator_gantt` to visualize thread utilization

## Utilities

```bash
verilator                # Main compiler
verilator_coverage       # Coverage analysis
verilator_gantt          # Visualize multithreaded execution
verilator_ccache_report  # ccache statistics
verilator_difftree       # Compare AST trees
verilator_profcfunc      # Profile C function performance
verilator_includer       # Process includes
```

## Important Files

- `Changes`: Release changelog (updated by maintainers from commit messages)
- `docs/internals.rst`: Detailed developer documentation
- `docs/CONTRIBUTING.rst`: Contribution guidelines
- `.clang-format`: C++ code formatting rules
- `.clang-tidy`: Static analysis configuration
- `configure.ac`: Autoconf configuration source

## Resources

- Manual: https://verilator.org/verilator_doc.html
- Forum: https://verilator.org/forum
- Issues: https://verilator.org/issues
- Internals presentation: https://www.veripool.org
