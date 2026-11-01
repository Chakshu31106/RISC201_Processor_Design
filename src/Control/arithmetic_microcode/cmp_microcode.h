
#ifndef CMP_MICROCODE_H
#define CMP_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildCmpRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("CMP", isa::CMP);

    // Read the first source operand into A.
    b.mmov(
        static_cast<int>(IR::RS1),
        static_cast<int>(IR::REG_SRC)
    );

    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::A),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    // Choose register mode or immediate mode.
    b.mbeq(
        static_cast<int>(IR::I),
        1,
        "CMP_IMMEDIATE"
    );

    // Register mode: B = R[RS2].
    b.mmov(
        static_cast<int>(IR::RS2),
        static_cast<int>(IR::REG_SRC)
    );

    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::B),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    b.mb("CMP_EXECUTE");

    // Immediate mode: B = immediate.
    b.label("CMP_IMMEDIATE");

    b.mmov(
        static_cast<int>(IR::IMMX),
        static_cast<int>(IR::B)
    );

    // Compare A and B; the ALU dispatch updates the flags.
    b.label("CMP_EXECUTE");

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::CMP
    );

    // CMP does not write an architectural destination register.
    b.mb("MICROCODE_END");

    b.finishRoutine();
}

} // namespace control

#endif
