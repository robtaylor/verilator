// Test harness for CXXRTL server POC

#include "Vcounter.h"
#include "verilated.h"
#include "verilated_cxxrtl_server.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

int main(int argc, char** argv) {
    // Create context
    const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};
    contextp->commandArgs(argc, argv);
    contextp->debug(0);
    contextp->randReset(2);
    contextp->traceEverOn(true);

    // Create model
    const std::unique_ptr<Vcounter> top{new Vcounter{contextp.get(), "TOP"}};

    // Create CXXRTL server
    VlCxxrtlServer server;

    // Get symbols - need to access internal symbol table
    // For POC, we pass nullptr and will use dummy data
    server.start(12345, top.get(), nullptr);

    std::cout << "Simulation started with CXXRTL server on port 12345\n";
    std::cout << "Connect with: python3 test_client.py\n";
    std::cout << "Running for 10 seconds...\n";

    // Run simulation
    top->rst = 1;
    top->clk = 0;

    for (int i = 0; i < 100 && !contextp->gotFinish(); i++) {
        // Toggle clock
        top->clk = !top->clk;

        // Deassert reset after a few cycles
        if (i == 10) top->rst = 0;

        // Evaluate
        top->eval();

        // Advance time
        contextp->timeInc(1);

        // Sleep a bit so clients can connect
        if (i < 20) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (i % 10 == 0) {
            std::cout << "Time=" << contextp->time()
                      << " clk=" << (int)top->clk
                      << " rst=" << (int)top->rst
                      << " count=" << (int)top->count << "\n";
        }
    }

    // Cleanup
    std::cout << "Stopping server...\n";
    server.stop();

    top->final();

    std::cout << "Simulation complete\n";
    return 0;
}
