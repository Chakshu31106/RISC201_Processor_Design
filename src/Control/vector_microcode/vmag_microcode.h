
#ifndef VMAG_MICROCODE_H
#define VMAG_MICROCODE_H

#include "vector_helpers.h"

namespace control {

inline void buildVmagRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("VMAG", isa::VMAG);

    // -------------------------------------------------
    // X component: x*x
    // -------------------------------------------------
    emitVectorRegisterSelection(
        b, IR::RS1, 0, IR::REG_SRC, "VMAG_X"
    );

    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::A),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    // Use the same value as both FPU operands.
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::FMUL
    );

    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::TMP1)
    );

    // -------------------------------------------------
    // Y component: y*y, then accumulate
    // -------------------------------------------------
    emitVectorRegisterSelection(
        b, IR::RS1, 1, IR::REG_SRC, "VMAG_Y"
    );

    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::A),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::FMUL
    );

    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::B)
    );

    b.mmov(
        static_cast<int>(IR::TMP1),
        static_cast<int>(IR::A)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::FADD
    );

    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::TMP1)
    );

    // -------------------------------------------------
    // Z component: z*z, then accumulate
    // -------------------------------------------------
    emitVectorRegisterSelection(
        b, IR::RS1, 2, IR::REG_SRC, "VMAG_Z"
    );

    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::A),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::FMUL
    );

    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::B)
    );

    b.mmov(
        static_cast<int>(IR::TMP1),
        static_cast<int>(IR::A)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::FADD
    );

    // -------------------------------------------------
    // Magnitude = sqrt(x*x + y*y + z*z)
    // -------------------------------------------------
    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::A)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::SQRT
    );

    // -------------------------------------------------
    // Write scalar result to architectural Rd
    // -------------------------------------------------
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
