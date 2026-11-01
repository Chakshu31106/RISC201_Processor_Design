
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

    // Test 1: R1 = 15, immediate = 10 => greater than.
    cu.writeRegister(1, 15);

    std::uint64_t instruction =
        (static_cast<std::uint64_t>(control::isa::CMP) << 57) |
        (static_cast<std::uint64_t>(3) << 52) |
        (static_cast<std::uint64_t>(1) << 47) |
        (static_cast<std::uint64_t>(1) << 41) |
        static_cast<std::uint64_t>(10);

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    assert(cu.readInternal(control::InternalRegister::FLAG_E) == 0);
    assert(cu.readInternal(control::InternalRegister::FLAG_GT) == 1);
    assert(cu.readInternal(control::InternalRegister::FLAG_ST) == 0);

    std::cout << "Immediate greater-than test passed!\n";

    // Test 2: R1 = 5, immediate = 10 => less than.
    cu.writeRegister(1, 5);

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    assert(cu.readInternal(control::InternalRegister::FLAG_E) == 0);
    assert(cu.readInternal(control::InternalRegister::FLAG_GT) == 0);
    assert(cu.readInternal(control::InternalRegister::FLAG_ST) == 1);

    std::cout << "Immediate less-than test passed!\n";

    // Test 3: R1 = 10, immediate = 10 => equal.
    cu.writeRegister(1, 10);

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    assert(cu.readInternal(control::InternalRegister::FLAG_E) == 1);
    assert(cu.readInternal(control::InternalRegister::FLAG_GT) == 0);
    assert(cu.readInternal(control::InternalRegister::FLAG_ST) == 0);

    std::cout << "Immediate equality test passed!\n";
    std::cout << "All CMP immediate tests passed!\n";

    return 0;
}
