
#include <cstdint>
#include <iostream>
#include <cassert>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

int main() {
    try {
        control::ControlUnit cu;

        // Build and load the microcode program.
        control::MicrocodeProgram program =
            control::buildDefaultMicrocode();

        cu.loadProgram(program);

        // Initialize architectural registers.
        cu.writeRegister(1, 10);
        cu.writeRegister(2, 20);

        // Test 1: Register mode: R3 = R1 + R2.
        std::uint64_t registerInstruction =
            (static_cast<std::uint64_t>(control::isa::ADD) << 57)
            | (static_cast<std::uint64_t>(3) << 52)
            | (static_cast<std::uint64_t>(1) << 47)
            | (static_cast<std::uint64_t>(2) << 42);

        cu.loadInstruction(registerInstruction, 0);
        cu.executeInstruction();

        const std::int32_t registerResult = cu.readRegister(3);

        std::cout << "Register mode: R3 = "
                  << registerResult << '\n';

        if (registerResult != 30) {
            std::cerr << "FAIL: Register-mode ADD\n";
            return 1;
        }

        // Test 2: Immediate mode: R4 = R1 + 5.
        const std::uint64_t immediateInstruction =
            (static_cast<std::uint64_t>(control::isa::ADD) << 57)
            | (static_cast<std::uint64_t>(4) << 52)
            | (static_cast<std::uint64_t>(1) << 47)
            | (static_cast<std::uint64_t>(1) << 41)
            | static_cast<std::uint64_t>(5);

        cu.loadInstruction(immediateInstruction, 0);
        cu.executeInstruction();

        assert(cu.readInternal(control::InternalRegister::PC) == 8);

        const std::int32_t immediateResult = cu.readRegister(4);

        std::cout << "Immediate mode: R4 = "
                  << immediateResult << '\n';

        if (immediateResult != 15) {
            std::cerr << "FAIL: Immediate-mode ADD\n";
            return 1;
        }

        std::cout << "Both ADD tests passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
