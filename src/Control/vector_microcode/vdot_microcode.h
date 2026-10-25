
#ifndef VDOT_MICROCODE_H
#define VDOT_MICROCODE_H

#include "vector_helpers.h"

namespace control {

inline void buildVdotRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("VDOT", isa::VDOT);

    // Component X: multiply the selected source components.
    emitVectorRegisterSelection(
        b, IR::RS1, 0, IR::REG_SRC, "VDOT_S1_X"
    );
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::A),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    emitVectorRegisterSelection(
        b, IR::RS2, 0, IR::REG_SRC, "VDOT_S2_X"
    );
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::B),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::FMUL
    );

    // Save the first product as the accumulator.
    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::TMP1)
    );

    // Component Y: multiply the selected source components.
    emitVectorRegisterSelection(
        b, IR::RS1, 1, IR::REG_SRC, "VDOT_S1_Y"
    );
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::A),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    emitVectorRegisterSelection(
        b, IR::RS2, 1, IR::REG_SRC, "VDOT_S2_Y"
    );
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::B),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::FMUL
    );

    // Accumulate the second product.
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

    // Component Z: multiply the selected source components.
    emitVectorRegisterSelection(
        b, IR::RS1, 2, IR::REG_SRC, "VDOT_S1_Z"
    );
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::A),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    emitVectorRegisterSelection(
        b, IR::RS2, 2, IR::REG_SRC, "VDOT_S2_Z"
    );
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::B),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::FMUL
    );

    // Add the third product to the accumulated result.
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

    // Write the scalar result to architectural register rd.
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
