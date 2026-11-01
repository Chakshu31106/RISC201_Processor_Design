#ifndef CLR_MICROCODE_H
#define CLR_MICROCODE_H

#include "stack_helpers.h"

namespace control {

inline void buildClrRoutine(MicrocodeBuilder& b) {
    using namespace stack_detail;
    b.beginRoutine("CLR", isa::CLR);
    // Reset the pointer to the empty-stack boundary; memory is intentionally untouched.
    imm(b, InternalRegister::REG_VAL, 65536);
    writeArch(b, 30, InternalRegister::REG_VAL);
    b.mb("MICROCODE_END"); b.finishRoutine();
}

} // namespace control

#endif
