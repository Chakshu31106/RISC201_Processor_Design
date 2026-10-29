#ifndef PEEK_MICROCODE_H
#define PEEK_MICROCODE_H

#include "stack_helpers.h"

namespace control {

inline void buildPeekRoutine(MicrocodeBuilder& b) {
    using namespace stack_detail;
    b.beginRoutine("PEEK", isa::PEEK);
    readArch(b, 30, InternalRegister::TMP1);
    b.mmov(static_cast<int>(InternalRegister::TMP1), static_cast<int>(InternalRegister::TMP4),
           UnitId::MEMORY, static_cast<int>(MemoryAction::READ_WORD));
    b.mmov(static_cast<int>(InternalRegister::TMP4), static_cast<int>(InternalRegister::REG_VAL));
    b.mmov(static_cast<int>(InternalRegister::RD), static_cast<int>(InternalRegister::REG_SRC));
    b.mmov(static_cast<int>(InternalRegister::REG_SRC), static_cast<int>(InternalRegister::REG_VAL),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::WRITE));
    b.mb("MICROCODE_END"); b.finishRoutine();
}

} // namespace control

#endif
