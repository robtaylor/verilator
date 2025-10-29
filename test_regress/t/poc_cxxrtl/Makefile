# Makefile for CXXRTL server POC test

# Find Verilator root
VERILATOR_ROOT ?= $(shell pwd)/../../..

# Verilator flags
VERILATOR_FLAGS = --cc --exe --build -j 0 --Mdir obj_dir
VERILATOR_FLAGS += -I$(VERILATOR_ROOT)/include
VERILATOR_FLAGS += --trace

# Source files
VERILOG_SOURCES = counter.v
CPP_SOURCES = sim_main.cpp
CPP_SOURCES += $(VERILATOR_ROOT)/include/verilated_cxxrtl_server.cpp

# Output
TARGET = obj_dir/Vcounter

.PHONY: all build run test clean check-verilator

all: build

check-verilator:
	@if [ ! -f "$(VERILATOR_ROOT)/bin/verilator_bin" ]; then \
		echo "ERROR: Verilator not built. Please run:"; \
		echo "  cd $(VERILATOR_ROOT)"; \
		echo "  autoconf && ./configure && make -j\$$(nproc)"; \
		exit 1; \
	fi

build: check-verilator $(TARGET)

$(TARGET): check-verilator $(VERILOG_SOURCES) $(CPP_SOURCES)
	$(VERILATOR_ROOT)/bin/verilator $(VERILATOR_FLAGS) $(VERILOG_SOURCES) $(CPP_SOURCES)

run: build
	@echo "Starting simulation with CXXRTL server..."
	@echo "Open another terminal and run: python3 test_client.py"
	./$(TARGET)

test: build
	@echo "Starting simulation in background..."
	./$(TARGET) &
	@sleep 2
	@echo "Running test client..."
	python3 test_client.py
	@echo "Waiting for simulation to finish..."
	@sleep 1
	@killall Vcounter 2>/dev/null || true

clean:
	rm -rf obj_dir
	rm -f *.vcd

.PHONY: help
help:
	@echo "CXXRTL Server POC Test"
	@echo ""
	@echo "Targets:"
	@echo "  make build  - Build the simulation"
	@echo "  make run    - Run simulation (manually run test_client.py in another terminal)"
	@echo "  make test   - Automated test (run simulation and client)"
	@echo "  make clean  - Clean build artifacts"
	@echo ""
	@echo "Environment Variables:"
	@echo "  VERILATOR_ROOT - Path to Verilator build/install"
	@echo "                   Default: ../../.. (relative path)"
	@echo ""
	@echo "Examples:"
	@echo "  # Use in-tree build (default)"
	@echo "  make test"
	@echo ""
	@echo "  # Use out-of-tree build"
	@echo "  VERILATOR_ROOT=/path/to/verilator make test"
	@echo ""
	@echo "  # Use system install"
	@echo "  VERILATOR_ROOT=/usr/local/share/verilator make test"
