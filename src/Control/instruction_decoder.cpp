
#include "instruction_decoder.h"
#include "isa_opcodes.h"

namespace control {

std::int64_t InstructionDecoder::signExtend(
    std::uint64_t value,
    unsigned width
) {
    const std::uint64_t signBit = std::uint64_t{1} << (width - 1);
    const std::uint64_t mask = (std::uint64_t{1} << width) - 1;

    value &= mask;
 
    if ((value & signBit) != 0) {
        value |= ~mask;
    }

    return static_cast<std::int64_t>(value);
}

bool InstructionDecoder::isBranchOpcode(std::uint8_t opcode) {
    switch (opcode) {
        case isa::CALL:
        case isa::RET:
        case isa::B:
        case isa::BEQ:
        case isa::BGT:
        case isa::BST:
            return true;

        default:
            return false;
    }
}

DecodedInstruction InstructionDecoder::decode(
    std::uint64_t instruction
) {
    DecodedInstruction result{};

    // Common fields.
    result.opcode = static_cast<std::uint8_t>(
        (instruction >> 57) & 0x7F
    );

    result.rd = static_cast<std::uint8_t>(
        (instruction >> 52) & 0x1F
    );

    result.rs1 = static_cast<std::uint8_t>(
        (instruction >> 47) & 0x1F
    );

    result.rs2 = static_cast<std::uint8_t>(
        (instruction >> 42) & 0x1F
    );

    result.isBranch = isBranchOpcode(result.opcode);

    if (result.isBranch) {
        // Branch offset occupies bits [41:0].
        result.branchOffset = signExtend(
            instruction & 0x3FFFFFFFFFFULL,
            42
        );

        result.immediateMode = false;
    } else {
        // Ordinary instruction: I is bit 41.
        result.immediateMode =
            ((instruction >> 41) & 1ULL) != 0;

        // Ordinary immediate occupies bits [40:0].
        result.immediate = signExtend(
            instruction & 0x1FFFFFFFFFFULL,
            41
        );
    }

    return result;
}

} // namespace control
