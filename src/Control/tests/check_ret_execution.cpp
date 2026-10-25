
#include <cstdint>
#include <iostream>
#include <exception>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"
#include "internal_registers.h"

int main() {
    try {
        control::ControlUnit cu;
        cu.loadProgram(control::buildDefaultMicrocode());

        // Set R31 to the address where RET should return.
        constexpr std::int32_t returnAddress = 200;
        cu.writeRegister(31, returnAddress);

        // Encode RET (no operands are needed).
        const std::uint64_t instruction =
            static_cast<std::uint64_t>(control::isa::RET) << 57;

        cu.loadInstruction(instruction, 100);
        cu.executeInstruction();

        const auto pc =
            cu.readInternal(control::InternalRegister::PC);

        std::cout << "Expected PC: " << returnAddress << '\n';
        std::cout << "Actual PC:   " << pc << '\n';

        if (pc != returnAddress) {
            std::cerr << "FAIL: RET did not restore the return address.\n";
            return 1;
        }

        std::cout << "All RET tests passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
