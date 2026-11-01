
#ifndef INSTRUCTION_DECODER_H
#define INSTRUCTION_DECODER_H

#include <cstdint>

namespace control {

struct DecodedInstruction {
    std::uint8_t opcode = 0;
    std::uint8_t rd = 0;
    std::uint8_t rs1 = 0;
    std::uint8_t rs2 = 0;
    bool immediateMode = false; 

    // Sign-extended 41-bit immediate for ordinary instructions.
    std::int64_t immediate = 0;

    // Sign-extended 42-bit offset for branch instructions.
    std::int64_t branchOffset = 0;

    bool isBranch = false;
};

class InstructionDecoder {
public:
    static DecodedInstruction decode(std::uint64_t instruction);

private:
    static std::int64_t signExtend(
        std::uint64_t value,
        unsigned width
    );

    static bool isBranchOpcode(std::uint8_t opcode);
};

} // namespace control

#endif
