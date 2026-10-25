
#include <cassert>
#include <cstdint>

#include "decoded_registers.h"
#include "isa_opcodes.h"

int main() {
    using namespace control;

    InternalRegisterValues registers{};

    // ADD r3, r5, r7
    const std::uint64_t instruction =
        (static_cast<std::uint64_t>(isa::ADD) << 57) |
        (std::uint64_t{3} << 52) |
        (std::uint64_t{5} << 47) |
        (std::uint64_t{7} << 42);

    loadDecodedRegisters(instruction, registers);

    assert(registers[registerIndex(InternalRegister::IR)] ==
           static_cast<std::int64_t>(instruction));

    assert(registers[registerIndex(InternalRegister::I)] == 0);
    assert(registers[registerIndex(InternalRegister::RD)] == 3);
    assert(registers[registerIndex(InternalRegister::RS1)] == 5);
    assert(registers[registerIndex(InternalRegister::RS2)] == 7);
    assert(registers[registerIndex(InternalRegister::IMMX)] == 0);
    assert(registers[registerIndex(InternalRegister::BRANCH_TARGET)] == 0);

    // B with a signed offset of -1.
    const std::uint64_t branchInstruction =
        (static_cast<std::uint64_t>(isa::B) << 57) |
        0x3FFFFFFFFFFULL;

    loadDecodedRegisters(branchInstruction, registers);

    assert(registers[registerIndex(InternalRegister::I)] == 0);
    assert(registers[registerIndex(InternalRegister::BRANCH_TARGET)] == -1);

    return 0;
}
