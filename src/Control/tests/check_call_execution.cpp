
#include <cassert>
#include <cstdint>
#include <iostream>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"
#include "internal_registers.h"

int main() {
    control::ControlUnit cu;
    cu.loadProgram(control::buildDefaultMicrocode());

    // CALL with offset +20, at instruction address 100.
    const std::uint64_t instruction =
        (static_cast<std::uint64_t>(control::isa::CALL) << 57) |
        static_cast<std::uint64_t>(20);

    cu.loadInstruction(instruction, 100);
    cu.executeInstruction();

    // Return address must be the next instruction.
    assert(cu.readRegister(31) == 108);

    // Target address must be PC + offset = 120.
    assert(
        cu.readInternal(control::InternalRegister::PC) == 120
    );

    std::cout << "CALL return address test passed!\n";
    std::cout << "CALL target address test passed!\n";
    std::cout << "All CALL tests passed!\n";

    return 0;
}
