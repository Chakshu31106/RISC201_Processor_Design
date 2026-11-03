#include <cassert>
#include <cstdint>
#include <iostream>

#include "../control_unit.h"
#include "../microcode_setup.h"
#include "../isa_opcodes.h"

// mov rd, #imm: I bit (bit 41) = 1, mode (bits 40:39) = 0, 39-bit immediate.
static std::uint64_t movImmediate(unsigned rd, std::int64_t imm) {
    return (std::uint64_t{control::isa::MOV} << 57) |
           (std::uint64_t{rd} << 52) |
           (std::uint64_t{1} << 41) |
           (static_cast<std::uint64_t>(imm) & 0x7FFFFFFFFFULL);
}

int main() {
    control::ControlUnit cu;
    cu.loadProgram(control::buildDefaultMicrocode());

    // Test 1: positive immediate.
    cu.loadInstruction(movImmediate(5, 123), 0);
    cu.executeInstruction();
    assert(cu.readRegister(5) == 123);
    std::cout << "Test 1 passed: mov r5, #123\n";

    // Test 2: negative immediate.
    cu.loadInstruction(movImmediate(6, -456), 8);
    cu.executeInstruction();
    assert(cu.readRegister(6) == -456);
    std::cout << "Test 2 passed: mov r6, #-456\n";

    // Test 3: the register form still works next to the immediate form.
    cu.writeRegister(2, 777);
    const std::uint64_t regForm =
        (std::uint64_t{control::isa::MOV} << 57) | (std::uint64_t{7} << 52) | (std::uint64_t{2} << 42);
    cu.loadInstruction(regForm, 16);
    cu.executeInstruction();
    assert(cu.readRegister(7) == 777);
    std::cout << "Test 3 passed: register form unchanged\n";

    std::cout << "All MOV immediate tests passed.\n";
    return 0;
}

