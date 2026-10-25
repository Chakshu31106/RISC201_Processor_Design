
#ifndef LD_MICROCODE_H
#define LD_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"
#include "../microinstruction.h"

namespace control {

inline void buildLdRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("LD", isa::LD);

    // Step 1: Read the base register R[rs1].
    b.mmov(
        static_cast<int>(IR::RS1),
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

    // Step 3: Move the sign-extended immediate into REG_SRC.
    b.mmov(
        static_cast<int>(IR::IMMX),
        static_cast<int>(IR::REG_SRC)
    );

    // Step 4: Calculate effective address = R[rs1] + immediate.
    b.mmov(
        static_cast<int>(IR::MAR),
        static_cast<int>(IR::REG_SRC),
        UnitId::ALU,
        isa::ADD
    );

    // Step 5: Save the effective address in MAR.
    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::MAR),
        UnitId::NONE
    );

    // Step 6: Read memory[MAR] into MDR.
    b.mmov(
        static_cast<int>(IR::MAR),
        static_cast<int>(IR::MDR),
        UnitId::MEMORY,
        static_cast<int>(MemoryAction::READ_WORD)
    );

    // Step 7: Select destination architectural register rd.
    b.mmov(
        static_cast<int>(IR::RD),
        static_cast<int>(IR::REG_SRC)
    );

    // Step 8: Move the loaded value into REG_VAL.
    // IMPORTANT: REGISTER_FILE WRITE takes its value from REG_VAL.
    b.mmov(
        static_cast<int>(IR::MDR),
        static_cast<int>(IR::REG_VAL),
        UnitId::NONE
    );

    // Step 9: Write REG_VAL into architectural register R[rd].
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::REG_VAL),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::WRITE)
    );

    // Step 10: Return to the shared microcode preamble.
    b.mb("MICROCODE_END");

    b.finishRoutine();
}

} // namespace control

#endif
