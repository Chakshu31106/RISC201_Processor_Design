
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

        // Build and load the microcode program.
        control::MicrocodeProgram program =
            control::buildDefaultMicrocode();

        cu.loadProgram(program);

        // Initialize architectural register R1.
        cu.writeRegister(1, 10);

        // Test: INCR R1 means R1 = R1 + 1.
        const std::uint64_t instruction =
            (static_cast<std::uint64_t>(control::isa::INCR) << 57)
            | (static_cast<std::uint64_t>(1) << 47);

        cu.loadInstruction(instruction, 0);
        cu.executeInstruction();

        const std::int32_t result = cu.readRegister(1);

        std::cout << "INCR mode: R1 = "
                  << result << '\n';

        assert(result == 11);
        assert(cu.readInternal(control::InternalRegister::PC) == 8);

        std::cout << "INCR execution test passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
