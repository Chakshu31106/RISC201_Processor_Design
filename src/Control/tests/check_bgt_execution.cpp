
#include <cassert>
#include <cstdint>
#include <iostream>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"
#include "internal_registers.h"

int main() {
    // Encode BGT with a branch offset of +20.
    const std::int64_t offset = 20;

    const std::uint64_t encodedOffset =
        static_cast<std::uint64_t>(offset) & 0x3FFFFFFFFFFULL;

    const std::uint64_t instruction =
        (static_cast<std::uint64_t>(control::isa::BGT) << 57) |
        encodedOffset;

    // Test 1: Branch taken when FLAG_GT == 1.
    {
        control::ControlUnit cu;
        cu.loadProgram(control::buildDefaultMicrocode());
        cu.loadInstruction(instruction, 100);

        cu.writeInternal(control::InternalRegister::FLAG_GT, 1);
        cu.executeInstruction();

        assert(cu.readInternal(control::InternalRegister::PC) == 120);

        std::cout << "BGT taken test passed!\n";
    }

    // Test 2: Branch not taken when FLAG_GT == 0.
    {
        control::ControlUnit cu;
        cu.loadProgram(control::buildDefaultMicrocode());
        cu.loadInstruction(instruction, 100);

        cu.writeInternal(control::InternalRegister::FLAG_GT, 0);
        cu.executeInstruction();

        assert(cu.readInternal(control::InternalRegister::PC) == 108);

        std::cout << "BGT not-taken test passed!\n";
    }

    std::cout << "All BGT microcode tests passed!\n";

    return 0;
}
