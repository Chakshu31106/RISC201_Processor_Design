
#include <cstdint>
#include <iostream>
#include <limits>
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

        auto encodeCbit = [](int rd, int rs1, int bit_idx) {
            return
                (static_cast<std::uint64_t>(control::isa::CBIT) << 57) |
                (static_cast<std::uint64_t>(rd) << 52) |
                (static_cast<std::uint64_t>(rs1) << 47) |
                (static_cast<std::uint64_t>(bit_idx) << 42);
        };

        auto runCbit = [&](std::int32_t value,
                           int bit_idx,
                           std::int32_t expected) {
            cu.writeRegister(1, value);

            const std::uint64_t instruction =
                encodeCbit(3, 1, bit_idx);

            cu.loadInstruction(instruction, 0);
            cu.executeInstruction();

            const std::int32_t result = cu.readRegister(3);

            std::cout << "CBIT(" << value
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

        if (!runCbit(5, 0, 4))
            return 1;

        if (!runCbit(5, 2, 1))
            return 1;

        if (!runCbit(15, 1, 13))
            return 1;

        if (!runCbit(0, 3, 0))
            return 1;

        if (!runCbit(
                -1, 31,
                std::numeric_limits<std::int32_t>::max()))
            return 1;

        std::cout << "All CBIT tests passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
