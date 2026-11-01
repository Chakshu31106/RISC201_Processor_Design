#include <cassert>
#include <cstdint>
#include <iostream>
#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"
#include "internal_registers.h"

static std::uint64_t regInstruction(std::uint8_t opcode, unsigned rd) {
    return (static_cast<std::uint64_t>(opcode) << 57) |
           (static_cast<std::uint64_t>(rd & 31U) << 52);
}
static std::uint64_t immInstruction(std::uint8_t opcode, std::int64_t imm) {
    return (static_cast<std::uint64_t>(opcode) << 57) |
           (std::uint64_t{1} << 41) |
           (static_cast<std::uint64_t>(imm) & 0x1FFFFFFFFFFULL);
}

int main() {
    control::ControlUnit cu;
    cu.loadProgram(control::buildDefaultMicrocode());
    assert(cu.readRegister(30) == 65536);

    cu.writeRegister(5, -123);
    cu.loadInstruction(regInstruction(control::isa::PUSH, 5), 0);
    cu.executeInstruction();
    assert(cu.readRegister(30) == 65532);
    assert(cu.readMemoryWord(65532) == static_cast<std::uint32_t>(-123));

    cu.loadInstruction(regInstruction(control::isa::PEEK, 6), 8);
    cu.executeInstruction();
    assert(cu.readRegister(6) == -123);
    assert(cu.readRegister(30) == 65532);

    cu.loadInstruction(regInstruction(control::isa::POP, 7), 16);
    cu.executeInstruction();
    assert(cu.readRegister(7) == -123);
    assert(cu.readRegister(30) == 65536);

    cu.writeRegister(30, 65532);
    cu.writeMemoryWord(65532, 77);
    cu.loadInstruction(immInstruction(control::isa::MPOP, 1), 24);
    cu.executeInstruction();
    assert(cu.readRegister(30) == 65536); // pointer moved by one 4-byte entry

    cu.writeRegister(30, 65520);
    cu.writeMemoryWord(65520, 111);
    cu.loadInstruction(static_cast<std::uint64_t>(control::isa::CLR) << 57, 32);
    cu.executeInstruction();
    assert(cu.readRegister(30) == 65536);
    assert(cu.readMemoryWord(65520) == 111); // CLR does not erase memory

    std::cout << "Stack PUSH/PEEK/POP/MPOP/CLR tests passed.\n";
}
