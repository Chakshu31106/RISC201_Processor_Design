
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "control_unit.h"
#include "isa_opcodes.h"

int main() {
    control::ControlUnit cu;
    control::MicrocodeProgram program;

    // Shared instruction preamble.
    std::vector<control::Microinstruction> preamble(3);

    preamble[0].type = control::MicroType::MLOAD_IR;
    preamble[1].type = control::MicroType::MDECODE;
    preamble[2].type = control::MicroType::MSWITCH;

    program.appendUnmapped(preamble);

    // Read R1 into REG_DATA, then write REG_DATA into R2.
    control::Microinstruction readR1{};
    readR1.type = control::MicroType::MMOV;
    readR1.src = static_cast<int>(control::InternalRegister::RS1);
    readR1.dest = static_cast<int>(control::InternalRegister::REG_DATA);
    readR1.unit = control::UnitId::REGISTER_FILE;
    readR1.opcode =
        static_cast<int>(control::RegisterFileAction::READ);

    control::Microinstruction writeR2{};
    writeR2.type = control::MicroType::MMOV;
    writeR2.src = static_cast<int>(control::InternalRegister::RS2);
    writeR2.dest = static_cast<int>(control::InternalRegister::REG_DATA);
    writeR2.unit = control::UnitId::REGISTER_FILE;
    writeR2.opcode =
        static_cast<int>(control::RegisterFileAction::WRITE);

    // Return to address zero to finish the instruction.
    control::Microinstruction finish{};
    finish.type = control::MicroType::MB;
    finish.seqAddr = 0;

    program.addRoutine(
        static_cast<std::uint8_t>(control::isa::ADD),
        std::vector<control::Microinstruction>{
            readR1, writeR2, finish
        }
    );

    cu.loadProgram(program);

    // Initialize architectural R1.
    cu.writeRegister(1, 123);

    // Encode ADD with rd=2, rs1=1, rs2=2, register mode.
    const std::uint64_t instruction =
        (static_cast<std::uint64_t>(control::isa::ADD) << 57) |
        (std::uint64_t{2} << 52) |
        (std::uint64_t{1} << 47) |
        (std::uint64_t{2} << 42);

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    assert(cu.readInternal(
        control::InternalRegister::REG_DATA
    ) == 123);

    assert(cu.readRegister(2) == 123);

    std::cout << "Register-file microcode test passed.\n";
    return 0;
}
