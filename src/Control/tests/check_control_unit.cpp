
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "control_unit.h"
#include "isa_opcodes.h"

int main() {
    using namespace control;

    MicrocodeProgram program; 

    // Register a simple routine for ADD (opcode 0x00).
    Microinstruction addInstruction{};
    addInstruction.type = MicroType::MMOV;

    program.addRoutine(
        isa::ADD,
        std::vector<Microinstruction>{addInstruction}
    );

    ControlUnit cu;
    cu.loadProgram(program);

    // Encode: ADD r3, r5, r7
    const std::uint64_t instruction =
        (static_cast<std::uint64_t>(isa::ADD) << 57) |
        (std::uint64_t{3} << 52) |
        (std::uint64_t{5} << 47) |
        (std::uint64_t{7} << 42);

    cu.loadInstruction(instruction, 100);

    // Check the architectural instruction address and IR.
    assert(cu.readInternal(InternalRegister::PC) == 100);
    assert(
        static_cast<std::uint64_t>(
            cu.readInternal(InternalRegister::IR)
        ) == instruction
    );

    // Check decoded register fields.
    assert(cu.readInternal(InternalRegister::RD) == 3);
    assert(cu.readInternal(InternalRegister::RS1) == 5);
    assert(cu.readInternal(InternalRegister::RS2) == 7);
    assert(cu.readInternal(InternalRegister::I) == 0);

    // ADD's microcode routine starts at address zero.
    assert(cu.getMicroPC() == 0);

    // An opcode without a registered routine must be rejected.
    const std::uint64_t missingRoutineInstruction =
        (static_cast<std::uint64_t>(isa::NOP) << 57);

    bool rejected = false;

    try {
        cu.loadInstruction(missingRoutineInstruction, 108);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    assert(rejected);

    return 0;
}
