
#include <cassert>
#include <cstdint>
#include <iostream>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

int main() {
    control::ControlUnit cu;

    cu.loadProgram(control::buildDefaultMicrocode());

    // Test 1: Copy a positive value.
    cu.writeRegister(2, 12345);

    const std::uint64_t mov1 =
        (std::uint64_t{control::isa::MOV} << 57) |
        (std::uint64_t{5} << 52) |
        (std::uint64_t{2} << 42);

    cu.loadInstruction(mov1, 0);
    cu.executeInstruction();

    assert(cu.readRegister(5) == 12345);
    assert(cu.readRegister(2) == 12345);

    std::cout << "Test 1 passed: register value copied\n";

    // Test 2: Copy a negative value.
    cu.writeRegister(7, -456);

    const std::uint64_t mov2 =
        (std::uint64_t{control::isa::MOV} << 57) |
        (std::uint64_t{9} << 52) |
        (std::uint64_t{7} << 42);

    cu.loadInstruction(mov2, 4);
    cu.executeInstruction();

    assert(cu.readRegister(9) == -456);
    assert(cu.readRegister(7) == -456);

    std::cout << "Test 2 passed: negative value copied\n";

    // Test 3: Copy zero.
    cu.writeRegister(10, 0);

    const std::uint64_t mov3 =
        (std::uint64_t{control::isa::MOV} << 57) |
        (std::uint64_t{11} << 52) |
        (std::uint64_t{10} << 42);

    cu.loadInstruction(mov3, 8);
    cu.executeInstruction();

    assert(cu.readRegister(11) == 0);

    std::cout << "Test 3 passed: zero copied\n";

    std::cout << "\nAll MOV tests passed.\n";

    return 0;
}
