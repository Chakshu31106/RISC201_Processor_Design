#ifndef RET_MICROCODE_H
#define RET_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"
#include "../stack_microcode/stack_helpers.h"

namespace control {

inline void buildRetRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;
    using namespace stack_detail;

    b.beginRoutine("RET", isa::RET);

    // The top item is this call's return address; the next item is the
    // caller's previous R31. Both are popped from the shared stack.
    popTmp4(b);
    b.mmov(static_cast<int>(IR::TMP4), static_cast<int>(IR::TMP3));
    popTmp4(b);

    b.mmov(static_cast<int>(IR::TMP4), static_cast<int>(IR::REG_VAL));
    writeArch(b, 31, IR::REG_VAL);
    b.mmov(static_cast<int>(IR::TMP3), static_cast<int>(IR::PC));

    b.mb("MICROCODE_END");
    b.finishRoutine();
}

} // namespace control

#endif
