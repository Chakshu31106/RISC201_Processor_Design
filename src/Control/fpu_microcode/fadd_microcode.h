
#ifndef FADD_MICROCODE_H
#define FADD_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildFaddRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("FADD", isa::FADD);

    // 1. Read rs1 into A.
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

    // 2. Read rs2 into B.
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

    // 3. Execute floating-point addition.
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::FADD
    );

    // 4. Write the result to rd.
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

    // 5. Return to the shared preamble.
    b.mb("MICROCODE_END");

    b.finishRoutine();
}

} // namespace control

#endif
