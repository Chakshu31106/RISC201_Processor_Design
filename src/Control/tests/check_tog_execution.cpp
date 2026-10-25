
#include <cstdint>
#include <iostream>
#include <limits>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

int main() {
    try {
        control::ControlUnit cu;

        // Build and load microcode.
        control::MicrocodeProgram program =
            control::buildDefaultMicrocode();

        cu.loadProgram(program);

        // Encode TOG using the bit-indexed format:
        // opcode: bits 63-57
        // rd:     bits 56-52
        // rs1:    bits 51-47
        // bit_idx: bits 46-42
        auto encodeTog = [](int rd, int rs1, int bit_idx) {
            return
                (static_cast<std::uint64_t>(control::isa::TOG) << 57) |
                (static_cast<std::uint64_t>(rd) << 52) |
                (static_cast<std::uint64_t>(rs1) << 47) |
                (static_cast<std::uint64_t>(bit_idx) << 42);
        };

        auto runTog = [&](std::int32_t value,
                          int bit_idx,
                          std::int32_t expected) {
            // R1 = input value.
            cu.writeRegister(1, value);

            // TOG R3, R1, bit_idx
            const std::uint64_t instruction =
                encodeTog(3, 1, bit_idx);

            cu.loadInstruction(instruction, 0);
            cu.executeInstruction();

            const std::int32_t result = cu.readRegister(3);

            std::cout << "TOG(" << value
                      << ", bit " << bit_idx
                      << ") = " << result << '\n';

            if (result != expected) {
                std::cerr << "FAIL: expected "
                          << expected << ", got "
                          << result << '\n';
                return false;
            }

            return true;
        };

        // Test 1: Toggle bit 0 of 5 (0101 -> 0100).
        if (!runTog(5, 0, 4))
            return 1;

        // Test 2: Toggle bit 2 of 5 (0101 -> 0001).
        if (!runTog(5, 2, 1))
            return 1;

        // Test 3: Toggle bit 31 of 0.
        const std::int32_t bit31 =
            std::numeric_limits<std::int32_t>::min();

        if (!runTog(0, 31, bit31))
            return 1;

        // Test 4: Toggle bit 2 when it is already set.
        if (!runTog(1, 2, 5))
            return 1;

        std::cout << "All TOG tests passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
