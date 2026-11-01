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
    cu.writeRegister(31, 200);

    // A CALL pushes the old R31 and its own return address. RET must
    // return to the latter and restore the former R31 value.
    const std::uint64_t call =
        (static_cast<std::uint64_t>(control::isa::CALL) << 57) | 20ULL;
    cu.loadInstruction(call, 100);
    cu.executeInstruction();
    assert(cu.readRegister(31) == 108);

    const std::uint64_t ret = static_cast<std::uint64_t>(control::isa::RET) << 57;
    cu.loadInstruction(ret, 120);
    cu.executeInstruction();
    assert(cu.readInternal(control::InternalRegister::PC) == 108);
    assert(cu.readRegister(31) == 200);
    assert(cu.readRegister(30) == 65536);

    std::cout << "RET return-address and R31 restoration tests passed.\n";
}
