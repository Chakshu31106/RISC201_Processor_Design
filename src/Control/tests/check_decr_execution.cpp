
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

        // Initialize architectural registers.
        cu.writeRegister(1, 5);

        // Test 1: Decrement R1 from 5 to 4.
        const std::uint64_t decrInstruction =
            (static_cast<std::uint64_t>(control::isa::DECR) << 57)
            | (static_cast<std::uint64_t>(1) << 47);

        cu.loadInstruction(decrInstruction, 0);
        cu.executeInstruction();

        const std::int32_t result = cu.readRegister(1);

        std::cout << "DECR test: R1 = "
                  << result << '\n';

        if (result != 4) {
            std::cerr << "FAIL: DECR did not change R1 from 5 to 4\n";
            return 1;
        }

        // Check that the PC advanced by one 64-bit instruction.
        assert(
            cu.readInternal(control::InternalRegister::PC) == 8
        );

        // Test 2: Decrement R1 from 0 to -1.
        cu.writeRegister(1, 0);

        cu.loadInstruction(decrInstruction, 0);
        cu.executeInstruction();

        const std::int32_t zeroResult = cu.readRegister(1);

        std::cout << "DECR zero test: R1 = "
                  << zeroResult << '\n';

        if (zeroResult != -1) {
            std::cerr << "FAIL: DECR did not change R1 from 0 to -1\n";
            return 1;
        }

        std::cout << "Both DECR tests passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
