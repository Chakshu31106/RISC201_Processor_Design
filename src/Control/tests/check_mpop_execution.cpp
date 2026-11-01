#include <cassert>
#include <cstdint>
#include <iostream>
#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

static std::uint64_t regInstruction(std::uint8_t opcode, unsigned rd) {
    return (static_cast<std::uint64_t>(opcode) << 57) |
           (static_cast<std::uint64_t>(rd & 31U) << 52);
}
static std::uint64_t immInstruction(std::uint8_t opcode, std::int64_t imm) {
    return (static_cast<std::uint64_t>(opcode) << 57) | (std::uint64_t{1} << 41) |
           (static_cast<std::uint64_t>(imm) & 0x1FFFFFFFFFFULL);
}

int main(){ control::ControlUnit cu; cu.loadProgram(control::buildDefaultMicrocode()); cu.writeRegister(30,65520); cu.loadInstruction(immInstruction(control::isa::MPOP,3),0); cu.executeInstruction(); assert(cu.readRegister(30)==65532); std::cout<<"MPOP passed\n"; }
