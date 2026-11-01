
#include <cstdint>
#include <iostream>
#include <cassert>
#include <exception>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

int main() {
    try {
        control::ControlUnit cu;

        control::MicrocodeProgram program =
            control::buildDefaultMicrocode();

        cu.loadProgram(program);

        // ABS R2, R1
        const std::uint64_t absInstruction =
            (static_cast<std::uint64_t>(control::isa::ABS) << 57)
            | (static_cast<std::uint64_t>(2) << 52)
            | (static_cast<std::uint64_t>(1) << 47);

        // Test 1: ABS(-5) = 5.
        cu.writeRegister(1, -5);
        cu.loadInstruction(absInstruction, 0);
        cu.executeInstruction();

        const std::int32_t negativeResult = cu.readRegister(2);

        std::cout << "ABS(-5) = " << negativeResult << '\n';

        if (negativeResult != 5) {
            std::cerr << "FAIL: ABS of a negative value\n";
            return 1;
        }

        // Test 2: ABS(5) = 5.
        cu.writeRegister(1, 5);
        cu.loadInstruction(absInstruction, 0);
        cu.executeInstruction();

        const std::int32_t positiveResult = cu.readRegister(2);

        std::cout << "ABS(5) = " << positiveResult << '\n';

        if (positiveResult != 5) {
            std::cerr << "FAIL: ABS of a positive value\n";
            return 1;
        }

        // Test 3: ABS(0) = 0.
        cu.writeRegister(1, 0);
        cu.loadInstruction(absInstruction, 0);
        cu.executeInstruction();

        const std::int32_t zeroResult = cu.readRegister(2);

        std::cout << "ABS(0) = " << zeroResult << '\n';

        if (zeroResult != 0) {
            std::cerr << "FAIL: ABS of zero\n";
            return 1;
        }

        std::cout << "All ABS execution tests passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
