
#ifndef VCRS_MICROCODE_H
#define VCRS_MICROCODE_H

#include "vector_helpers.h"

namespace control {

inline void buildVcrsRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("VCRS", isa::VCRS);

    // Save Va = (xa, ya, za) into TMP1, TMP2, TMP3.
    emitVectorRegisterSelection(
        b, IR::RS1, 0, IR::REG_SRC, "VCRS_AX"
    );
    b.mmov(static_cast<int>(IR::REG_SRC), static_cast<int>(IR::TMP4),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::READ));
    b.mmov(static_cast<int>(IR::TMP4), static_cast<int>(IR::TMP1));

    emitVectorRegisterSelection(
        b, IR::RS1, 1, IR::REG_SRC, "VCRS_AY"
    );
    b.mmov(static_cast<int>(IR::REG_SRC), static_cast<int>(IR::TMP4),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::READ));
    b.mmov(static_cast<int>(IR::TMP4), static_cast<int>(IR::TMP2));

    emitVectorRegisterSelection(
        b, IR::RS1, 2, IR::REG_SRC, "VCRS_AZ"
    );
    b.mmov(static_cast<int>(IR::REG_SRC), static_cast<int>(IR::TMP4),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::READ));
    b.mmov(static_cast<int>(IR::TMP4), static_cast<int>(IR::TMP3));

    // Save Vb = (xb, yb, zb) into TMP4, A_F, B_F.
    emitVectorRegisterSelection(
        b, IR::RS2, 0, IR::REG_SRC, "VCRS_BX"
    );
    b.mmov(static_cast<int>(IR::REG_SRC), static_cast<int>(IR::TMP4),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::READ));
    b.mmov(static_cast<int>(IR::TMP4), static_cast<int>(IR::A_F));

    emitVectorRegisterSelection(
        b, IR::RS2, 1, IR::REG_SRC, "VCRS_BY"
    );
    b.mmov(static_cast<int>(IR::REG_SRC), static_cast<int>(IR::TMP4),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::READ));
    b.mmov(static_cast<int>(IR::TMP4), static_cast<int>(IR::B_F));

    emitVectorRegisterSelection(
        b, IR::RS2, 2, IR::REG_SRC, "VCRS_BZ"
    );
    b.mmov(static_cast<int>(IR::REG_SRC), static_cast<int>(IR::TMP4),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::READ));
    b.mmov(static_cast<int>(IR::TMP4), static_cast<int>(IR::F_RESULT));

    // Important: the saved source components are:
    // xa=TMP1, ya=TMP2, za=TMP3,
    // xb=A_F, yb=B_F, zb=F_RESULT.
    //
    // Compute xd = ya*zb - za*yb.
    b.mmov(static_cast<int>(IR::TMP2), static_cast<int>(IR::F_RESULT),
           UnitId::FPU, isa::FMUL);
    b.mmov(static_cast<int>(IR::ALU_RESULT), static_cast<int>(IR::LD_RESULT));

    b.mmov(static_cast<int>(IR::TMP3), static_cast<int>(IR::B_F),
           UnitId::FPU, isa::FMUL);
    b.mmov(static_cast<int>(IR::LD_RESULT), static_cast<int>(IR::ALU_RESULT),
           UnitId::FPU, isa::FSUB);
    b.mmov(static_cast<int>(IR::ALU_RESULT), static_cast<int>(IR::LD_RESULT));

    // Compute yd = za*xb - xa*zb.
    b.mmov(static_cast<int>(IR::TMP3), static_cast<int>(IR::A_F),
           UnitId::FPU, isa::FMUL);
    b.mmov(static_cast<int>(IR::ALU_RESULT), static_cast<int>(IR::REG_DATA));

    b.mmov(static_cast<int>(IR::TMP1), static_cast<int>(IR::F_RESULT),
           UnitId::FPU, isa::FMUL);
    b.mmov(static_cast<int>(IR::REG_DATA), static_cast<int>(IR::ALU_RESULT),
           UnitId::FPU, isa::FSUB);
    b.mmov(static_cast<int>(IR::ALU_RESULT), static_cast<int>(IR::REG_VAL));

    // Compute zd = xa*yb - ya*xb.
    b.mmov(static_cast<int>(IR::TMP1), static_cast<int>(IR::B_F),
           UnitId::FPU, isa::FMUL);
    b.mmov(static_cast<int>(IR::ALU_RESULT), static_cast<int>(IR::REG_DATA));

    b.mmov(static_cast<int>(IR::TMP2), static_cast<int>(IR::A_F),
           UnitId::FPU, isa::FMUL);
    b.mmov(static_cast<int>(IR::REG_DATA), static_cast<int>(IR::ALU_RESULT),
           UnitId::FPU, isa::FSUB);
    b.mmov(static_cast<int>(IR::ALU_RESULT), static_cast<int>(IR::F_RESULT));

    // Only now write the destination vector, so aliasing is safe.
    emitVectorRegisterSelection(
        b, IR::RD, 0, IR::REG_SRC, "VCRS_DX"
    );
    b.mmov(static_cast<int>(IR::REG_SRC), static_cast<int>(IR::LD_RESULT),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::WRITE));

    emitVectorRegisterSelection(
        b, IR::RD, 1, IR::REG_SRC, "VCRS_DY"
    );
    b.mmov(static_cast<int>(IR::REG_SRC), static_cast<int>(IR::REG_VAL),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::WRITE));

    emitVectorRegisterSelection(
        b, IR::RD, 2, IR::REG_SRC, "VCRS_DZ"
    );
    b.mmov(static_cast<int>(IR::REG_SRC), static_cast<int>(IR::F_RESULT),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::WRITE));

    b.mb("MICROCODE_END");
    b.finishRoutine();
}

} // namespace control

#endif
