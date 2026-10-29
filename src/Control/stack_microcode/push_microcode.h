#ifndef PUSH_MICROCODE_H
#define PUSH_MICROCODE_H

#include "stack_helpers.h"

namespace control {

inline void buildPushRoutine(MicrocodeBuilder& b) {
    using namespace stack_detail;
    b.beginRoutine("PUSH", isa::PUSH);
    b.mmov(static_cast<int>(InternalRegister::RD), static_cast<int>(InternalRegister::REG_SRC));
    b.mmov(static_cast<int>(InternalRegister::REG_SRC), static_cast<int>(InternalRegister::TMP4),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::READ));
    pushTmp4(b);
    b.mb("MICROCODE_END"); b.finishRoutine();
}

} // namespace control

#endif
