#ifndef POP_MICROCODE_H
#define POP_MICROCODE_H

#include "stack_helpers.h"

namespace control {

inline void buildPopRoutine(MicrocodeBuilder& b) {
    using namespace stack_detail;
    b.beginRoutine("POP", isa::POP);
    popTmp4(b);
    b.mmov(static_cast<int>(InternalRegister::TMP4), static_cast<int>(InternalRegister::REG_VAL));
    b.mmov(static_cast<int>(InternalRegister::RD), static_cast<int>(InternalRegister::REG_SRC));
    b.mmov(static_cast<int>(InternalRegister::REG_SRC), static_cast<int>(InternalRegister::REG_VAL),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::WRITE));
    b.mb("MICROCODE_END"); b.finishRoutine();
}

} // namespace control

#endif
