#ifndef MPOP_MICROCODE_H
#define MPOP_MICROCODE_H

#include "stack_helpers.h"

namespace control {

inline void buildMpopRoutine(MicrocodeBuilder& b) {
    using namespace stack_detail;
    b.beginRoutine("MPOP", isa::MPOP);
    readArch(b, 30, InternalRegister::TMP1);
    b.mmov(static_cast<int>(InternalRegister::IMMX), static_cast<int>(InternalRegister::TMP2));
    imm(b, InternalRegister::TMP3, 4);
    b.mmov(static_cast<int>(InternalRegister::TMP2), static_cast<int>(InternalRegister::TMP3), UnitId::ALU, isa::MUL);
    b.mmov(static_cast<int>(InternalRegister::ALU_RESULT), static_cast<int>(InternalRegister::TMP2));
    b.mmov(static_cast<int>(InternalRegister::TMP1), static_cast<int>(InternalRegister::TMP2), UnitId::ALU, isa::ADD);
    b.mmov(static_cast<int>(InternalRegister::ALU_RESULT), static_cast<int>(InternalRegister::REG_VAL));
    writeArch(b, 30, InternalRegister::REG_VAL);
    b.mb("MICROCODE_END"); b.finishRoutine();
}

} // namespace control

#endif
