
#include <cstdint>
#include <iostream>
#include <exception>
#include <limits>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

int main() {
    try {
        control::ControlUnit cu;

        auto program = control::buildDefaultMicrocode();
        cu.loadProgram(program);

        const auto runPcnt = [&](int source, int destination) {
            const std::uint64_t instruction =
                (static_cast<std::uint64_t>(control::isa::PCNT) << 57)
                | (static_cast<std::uint64_t>(destination) << 52)
                | (static_cast<std::uint64_t>(source) << 47);

            cu.loadInstruction(instruction, 0);
            cu.executeInstruction();
            return cu.readRegister(destination);
        };

        // Test 1: PCNT(0) = 0.
        cu.writeRegister(1, 0);
        if (runPcnt(1, 2) != 0) {
            std::cerr << "FAIL: PCNT(0)\n";
            return 1;
        }

        // Test 2: PCNT(7) = 3.
        cu.writeRegister(1, 7);
        const auto result2 = runPcnt(1, 2);
        std::cout << "PCNT(7) = " << result2 << '\n';

        if (result2 != 3) {
            std::cerr << "FAIL: PCNT(7)\n";
            return 1;
        }

        // Test 3: PCNT(15) = 4.
        cu.writeRegister(1, 15);
        const auto result3 = runPcnt(1, 2);
        std::cout << "PCNT(15) = " << result3 << '\n';

        if (result3 != 4) {
            std::cerr << "FAIL: PCNT(15)\n";
            return 1;
        }

        // Test 4: All 32 bits set.
        cu.writeRegister(1, -1);
        const auto result4 = runPcnt(1, 2);
        std::cout << "PCNT(-1) = " << result4 << '\n';

        if (result4 != 32) {
            std::cerr << "FAIL: PCNT(-1)\n";
            return 1;
        }

        std::cout << "All PCNT tests passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
