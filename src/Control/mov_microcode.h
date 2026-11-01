
#ifndef MOV_MICROCODE_H
#define MOV_MICROCODE_H

#include "microcode_builder.h"
#include "isa_opcodes.h"
#include "internal_registers.h"
#include "microinstruction.h"

namespace control {

inline void buildMovRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("MOV", isa::MOV);

    // Step 1: Read the value in architectural register rs2.
    b.mmov(
        static_cast<int>(IR::RS2),
        static_cast<int>(IR::REG_VAL),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    // Step 2: Select the destination architectural register rd.
    b.mmov(
        static_cast<int>(IR::RD),
        static_cast<int>(IR::REG_SRC),
        UnitId::NONE
    );

    // Step 3: Write the value from REG_VAL into R[rd].
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::REG_VAL),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::WRITE)
    );

    // Step 4: Finish the instruction.
    b.mb("MICROCODE_END");

    b.finishRoutine();
}

} // namespace control

#endif
