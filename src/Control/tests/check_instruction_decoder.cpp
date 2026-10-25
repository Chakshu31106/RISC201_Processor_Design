
#include <cassert>
#include <cstdint>

#include "instruction_decoder.h"
#include "isa_opcodes.h"

int main() {
    using namespace control;

    // Test 1: ADD with rd=3, rs1=5, rs2=7, register mode.
    std::uint64_t addInstruction =
        (static_cast<std::uint64_t>(isa::ADD) << 57) |
        (std::uint64_t{3} << 52) |
        (std::uint64_t{5} << 47) |
        (std::uint64_t{7} << 42);

    DecodedInstruction add =
        InstructionDecoder::decode(addInstruction);

    assert(add.opcode == isa::ADD);
    assert(add.rd == 3);
    assert(add.rs1 == 5);
    assert(add.rs2 == 7);
    assert(!add.immediateMode);
    assert(!add.isBranch);

    // Test 2: ADD immediate, I=1 and immediate=10.
    std::uint64_t addImmediate =
        (static_cast<std::uint64_t>(isa::ADD) << 57) |
        (std::uint64_t{3} << 52) |
        (std::uint64_t{5} << 47) |
        (std::uint64_t{1} << 41) |
        std::uint64_t{10};

    DecodedInstruction addi =
        InstructionDecoder::decode(addImmediate);

    assert(addi.opcode == isa::ADD);
    assert(addi.immediateMode);
    assert(addi.immediate == 10);

    // Test 3: B with a branch offset of -1.
    // A 42-bit two's-complement representation of -1
    // contains 42 one-bits.
    std::uint64_t branchInstruction =
        (static_cast<std::uint64_t>(isa::B) << 57) |
        0x3FFFFFFFFFFULL;

    DecodedInstruction branch =
        InstructionDecoder::decode(branchInstruction);

    assert(branch.opcode == isa::B);
    assert(branch.isBranch);
    assert(branch.branchOffset == -1);

    return 0;
}
