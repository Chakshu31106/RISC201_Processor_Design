
#include <cstdint>
#include <iostream>
#include <exception>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

int main() {
    try {
        control::ControlUnit cu;

        auto program = control::buildDefaultMicrocode();
        cu.loadProgram(program);

        // Test 1: NOT 0 = -1 for a 32-bit signed integer.
        cu.writeRegister(1, 0);

        const std::uint64_t instruction1 =
            (static_cast<std::uint64_t>(control::isa::NOT) << 57)
            | (static_cast<std::uint64_t>(2) << 52)
            | (static_cast<std::uint64_t>(1) << 47);

        cu.loadInstruction(instruction1, 0);
        cu.executeInstruction();

        const std::int32_t result1 = cu.readRegister(2);

        std::cout << "NOT(0) = " << result1 << '\n';

        if (result1 != -1) {
            std::cerr << "FAIL: NOT(0)\n";
            return 1;
        }

        // Test 2: NOT 12 = -13 in 32-bit two's complement.
        cu.writeRegister(1, 12);

        const std::uint64_t instruction2 =
            (static_cast<std::uint64_t>(control::isa::NOT) << 57)
            | (static_cast<std::uint64_t>(3) << 52)
            | (static_cast<std::uint64_t>(1) << 47);

        cu.loadInstruction(instruction2, 0);
        cu.executeInstruction();

        const std::int32_t result2 = cu.readRegister(3);

        std::cout << "NOT(12) = " << result2 << '\n';

        if (result2 != -13) {
            std::cerr << "FAIL: NOT(12)\n";
            return 1;
        }

        // Test 3: NOT(-1) = 0.
        cu.writeRegister(1, -1);

        const std::uint64_t instruction3 =
            (static_cast<std::uint64_t>(control::isa::NOT) << 57)
            | (static_cast<std::uint64_t>(4) << 52)
            | (static_cast<std::uint64_t>(1) << 47);

        cu.loadInstruction(instruction3, 0);
        cu.executeInstruction();

        const std::int32_t result3 = cu.readRegister(4);

        std::cout << "NOT(-1) = " << result3 << '\n';

        if (result3 != 0) {
            std::cerr << "FAIL: NOT(-1)\n";
            return 1;
        }

        std::cout << "All NOT tests passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
