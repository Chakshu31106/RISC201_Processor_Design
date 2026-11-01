
#ifndef MOD_MICROCODE_H
#define MOD_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildModRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("MOD", isa::MOD);

    // Read the first source operand.
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
        "MOD_IMMEDIATE"
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

    b.mb("MOD_EXECUTE");

    // Immediate mode: B = immediate value.
    b.label("MOD_IMMEDIATE");

    b.mmov(
        static_cast<int>(IR::IMMX),
        static_cast<int>(IR::B)
    );

    // Execute the remainder operation.
    b.label("MOD_EXECUTE");

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::MOD
    );

    // Write the result to the destination register.
    b.mmov(
        static_cast<int>(IR::RD),
        static_cast<int>(IR::REG_SRC)
    );

    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::ALU_RESULT),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::WRITE)
    );

    b.mb("MICROCODE_END");

    b.finishRoutine();
}

} // namespace control

#endif
