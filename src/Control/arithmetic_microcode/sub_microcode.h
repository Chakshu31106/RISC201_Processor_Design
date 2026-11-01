
#ifndef SUB_MICROCODE_H
#define SUB_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildSubRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("SUB", isa::SUB);

    // Read rs1 into A.
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

    // If I == 1, use the immediate operand.
    b.mbeq(
        static_cast<int>(IR::I),
        1,
        "SUB_IMMEDIATE"
    );

    // Register mode: read rs2 into B.
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

    b.mb("SUB_EXECUTE");

    // Immediate mode: copy the immediate into B.
    b.label("SUB_IMMEDIATE");

    b.mmov(
        static_cast<int>(IR::IMMX),
        static_cast<int>(IR::B)
    );

    // Execute A - B.
    b.label("SUB_EXECUTE");

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::SUB
    );

    // Write the result to rd.
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

    // Return to the shared preamble.
    b.mb("MICROCODE_END");

    b.finishRoutine();
}

} // namespace control

#endif
