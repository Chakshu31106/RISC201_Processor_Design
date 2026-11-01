
#include "decoded_registers.h"

namespace control {

void loadDecodedRegisters(
    std::uint64_t instruction,
    InternalRegisterValues& registers
) {
    // Preserve the complete instruction in IR.
    registers[registerIndex(InternalRegister::IR)] =
        static_cast<std::int64_t>(instruction);

    const DecodedInstruction decoded =
        InstructionDecoder::decode(instruction);

    registers[registerIndex(InternalRegister::I)] =
        decoded.immediateMode ? 1 : 0;

    registers[registerIndex(InternalRegister::RD)] =
        decoded.rd;

    registers[registerIndex(InternalRegister::RS1)] =
        decoded.rs1;

    registers[registerIndex(InternalRegister::RS2)] =
        decoded.rs2;

    registers[registerIndex(InternalRegister::IMMX)] =
        decoded.immediate;

    // Store the signed PC-relative offset for now.
    // The microcode will calculate the final target after
    // the ISA's exact PC-relative convention is confirmed.
    if (decoded.isBranch) {
        registers[registerIndex(InternalRegister::BRANCH_TARGET)] =
            decoded.branchOffset;
    } else {
        registers[registerIndex(InternalRegister::BRANCH_TARGET)] = 0;
    }
}

} // namespace control
