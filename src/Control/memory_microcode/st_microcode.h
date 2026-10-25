
#ifndef ST_MICROCODE_H
#define ST_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"
#include "../microinstruction.h"

namespace control {

inline void buildStRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("ST", isa::ST);

    // Step 1: Read R[rd], the base address.
    b.mmov(
        static_cast<int>(IR::RD),
        static_cast<int>(IR::REG_SRC),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    // Step 2: Save the base address in MAR.
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::MAR),
        UnitId::NONE
    );

    // Step 3: Read R[rs1], the value to store.
    b.mmov(
        static_cast<int>(IR::RS1),
        static_cast<int>(IR::REG_SRC),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    // Step 4: Preserve the value to store in MDR.
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::MDR),
        UnitId::NONE
    );

    // Step 5: Load the sign-extended immediate.
    b.mmov(
        static_cast<int>(IR::IMMX),
        static_cast<int>(IR::REG_SRC),
        UnitId::NONE
    );

    // Step 6: Calculate effective address:
    // EA = R[rd] + sign-extended immediate.
    b.mmov(
        static_cast<int>(IR::MAR),
        static_cast<int>(IR::REG_SRC),
        UnitId::ALU,
        isa::ADD
    );

    // Step 7: Save the effective address in MAR.
    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::MAR),
        UnitId::NONE
    );

    // Step 8: Write MDR to memory[MAR].
    b.mmov(
        static_cast<int>(IR::MAR),
        static_cast<int>(IR::MDR),
        UnitId::MEMORY,
        static_cast<int>(MemoryAction::WRITE_WORD)
    );

    // Step 9: Finish the instruction.
    b.mb("MICROCODE_END");

    b.finishRoutine();
}

} // namespace control

#endif
