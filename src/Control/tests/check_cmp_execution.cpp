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

    // Compare 10 and 10: equality expected.
    cu.writeRegister(1, 10);
    cu.writeRegister(2, 10);

    std::uint64_t instruction =
        (static_cast<std::uint64_t>(control::isa::CMP) << 57) |
        (static_cast<std::uint64_t>(3) << 52) |
        (static_cast<std::uint64_t>(1) << 47) |
        (static_cast<std::uint64_t>(2) << 42);

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    assert(cu.readInternal(control::InternalRegister::FLAG_E) == 1);
    assert(cu.readInternal(control::InternalRegister::FLAG_GT) == 0);
    assert(cu.readInternal(control::InternalRegister::FLAG_ST) == 0);

    std::cout << "Equality test passed!\n";

    // Compare 20 and 10: greater-than expected.
    cu.writeRegister(1, 20);
    cu.writeRegister(2, 10);

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    assert(cu.readInternal(control::InternalRegister::FLAG_E) == 0);
    assert(cu.readInternal(control::InternalRegister::FLAG_GT) == 1);
    assert(cu.readInternal(control::InternalRegister::FLAG_ST) == 0);

    std::cout << "Greater-than test passed!\n";

    // Compare 5 and 10: less-than expected.
    cu.writeRegister(1, 5);
    cu.writeRegister(2, 10);

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    assert(cu.readInternal(control::InternalRegister::FLAG_E) == 0);
    assert(cu.readInternal(control::InternalRegister::FLAG_GT) == 0);
    assert(cu.readInternal(control::InternalRegister::FLAG_ST) == 1);

    std::cout << "Less-than test passed!\n";
    std::cout << "All CMP tests passed!\n";

    return 0;
}
