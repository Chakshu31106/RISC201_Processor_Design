#include <cassert>
#include <cstdint>
#include <iostream>
#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"
#include "internal_registers.h"

static std::uint64_t callInstruction(std::int64_t offset) {
    return (static_cast<std::uint64_t>(control::isa::CALL) << 57) |
           (static_cast<std::uint64_t>(offset) & 0x3FFFFFFFFFFULL);
}
static std::uint64_t retInstruction() {
    return static_cast<std::uint64_t>(control::isa::RET) << 57;
}

int main() {
    control::ControlUnit cu;
    cu.loadProgram(control::buildDefaultMicrocode());
    cu.writeRegister(31, 444);

    cu.loadInstruction(callInstruction(40), 100);
    cu.executeInstruction();
    assert(cu.readRegister(31) == 108);
    assert(cu.readRegister(30) == 65528);
    assert(cu.readMemoryWord(65528) == 108);
    assert(cu.readMemoryWord(65532) == 444);
    assert(cu.readInternal(control::InternalRegister::PC) == 140);

    // Nested CALL: preserve the outer call's R31 and return address.
    cu.loadInstruction(callInstruction(40), 140);
    cu.executeInstruction();
    assert(cu.readRegister(31) == 148);
    assert(cu.readInternal(control::InternalRegister::PC) == 180);

    cu.loadInstruction(retInstruction(), 180);
    cu.executeInstruction();
    assert(cu.readInternal(control::InternalRegister::PC) == 148);
    assert(cu.readRegister(31) == 108);

    cu.loadInstruction(retInstruction(), 148);
    cu.executeInstruction();
    assert(cu.readInternal(control::InternalRegister::PC) == 108);
    assert(cu.readRegister(31) == 444);
    assert(cu.readRegister(30) == 65536);

    std::cout << "Nested CALL/RET and R31 restoration tests passed.\n";
}
