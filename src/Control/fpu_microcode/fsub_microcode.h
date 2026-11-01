
#ifndef FSUB_MICROCODE_H
#define FSUB_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildFsubRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("FSUB", isa::FSUB);

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

    // Read rs2 into B.
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

    // Execute floating-point subtraction.
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::FSUB
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

    b.mb("MICROCODE_END");
    b.finishRoutine();
}

} // namespace control

#endif
