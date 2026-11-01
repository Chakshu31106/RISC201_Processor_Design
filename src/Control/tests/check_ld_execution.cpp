
#include <cassert>
#include <cstdint>
#include <iostream>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

int main() {
    control::ControlUnit cu;

    // Load microcode, including the LD routine.
    auto program = control::buildDefaultMicrocode();
    cu.loadProgram(program);

    const uint64_t immediate_flag = uint64_t(1) << 41;

    // --------------------------------------------------
    // Test 1: Positive offset
    // LD R5, [R1 + 8]
    // --------------------------------------------------
    cu.writeRegister(1, 100);
    cu.writeMemoryWord(108, 12345);

    uint64_t instruction =
        (uint64_t(control::isa::LD) << 57) |
        (uint64_t(5) << 52) |
        (uint64_t(1) << 47) |
        immediate_flag |
        uint64_t(8);

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    assert(cu.readRegister(5) == 12345);
    std::cout << "Test 1 passed: positive offset\n";

    // --------------------------------------------------
    // Test 2: Negative offset
    // LD R6, [R2 - 8]
    // --------------------------------------------------
    cu.writeRegister(2, 200);
    cu.writeMemoryWord(192, 54321);

    instruction =
        (uint64_t(control::isa::LD) << 57) |
        (uint64_t(6) << 52) |
        (uint64_t(2) << 47) |
        immediate_flag |
        ((uint64_t(1) << 41) - uint64_t(8));

    cu.loadInstruction(instruction, 8);
    cu.executeInstruction();

    assert(cu.readRegister(6) == 54321);
    std::cout << "Test 2 passed: negative offset\n";

    // --------------------------------------------------
    // Test 3: Zero offset
    // LD R7, [R3 + 0]
    // --------------------------------------------------
    cu.writeRegister(3, 300);
    cu.writeMemoryWord(300, 98765);

    instruction =
        (uint64_t(control::isa::LD) << 57) |
        (uint64_t(7) << 52) |
        (uint64_t(3) << 47) |
        immediate_flag;

    cu.loadInstruction(instruction, 16);
    cu.executeInstruction();

    assert(cu.readRegister(7) == 98765);
    std::cout << "Test 3 passed: zero offset\n";

    std::cout << "\nAll LD tests passed.\n";

    return 0;
}
