
#include <cassert>
#include <cstdint>
#include <iostream>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

int main() {
    control::ControlUnit cu;

    // Load the microcode program.
    const auto program = control::buildDefaultMicrocode();
    cu.loadProgram(program);

    // Confirm that NOP has a registered routine.
    assert(program.getEntryAddress(control::isa::NOP) >= 0);

    // Set up registers and memory with known values.
    cu.writeRegister(1, 1234);
    cu.writeRegister(2, -567);
    cu.writeMemoryWord(100, 98765);

    // Encode NOP: opcode occupies bits 63:57.
    const std::uint64_t instruction =
        std::uint64_t{control::isa::NOP} << 57;

    // Execute NOP.
    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    // NOP must not modify architectural registers or memory.
    assert(cu.readRegister(1) == 1234);
    assert(cu.readRegister(2) == -567);
    assert(cu.readMemoryWord(100) == 98765);

    std::cout << "Test 1 passed: NOP routine is registered\n";
    std::cout << "Test 2 passed: registers remain unchanged\n";
    std::cout << "Test 3 passed: memory remains unchanged\n";
    std::cout << "\nAll NOP tests passed.\n";

    return 0;
}
