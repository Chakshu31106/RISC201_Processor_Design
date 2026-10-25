
#include <cassert>
#include <cstdint>
#include <iostream>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"
#include "internal_registers.h"

int main() {
    // Encode B with a branch offset of +20.
    const std::int64_t offset = 20;

    const std::uint64_t encodedOffset =
        static_cast<std::uint64_t>(offset) & 0x3FFFFFFFFFFULL;

    const std::uint64_t instruction =
        (static_cast<std::uint64_t>(control::isa::B) << 57) |
        encodedOffset;

    control::ControlUnit cu;
    cu.loadProgram(control::buildDefaultMicrocode());

    // Load B at address 100.
    cu.loadInstruction(instruction, 100);

    cu.executeInstruction();

    // Unconditional branch: PC = 100 + 20.
    assert(cu.readInternal(control::InternalRegister::PC) == 120);

    std::cout << "B unconditional branch test passed!\n";
    std::cout << "All B microcode tests passed!\n";

    return 0;
}
