#ifndef CALL_MICROCODE_H
#define CALL_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"
#include "../stack_microcode/stack_helpers.h"

namespace control {

inline void buildCallRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;
    using namespace stack_detail;

    b.beginRoutine("CALL", isa::CALL);

    // Save the caller's R31 first, then the return address. Since the stack
    // grows down, the later return-address push becomes the top item.
    readArch(b, 31, IR::TMP4);
    pushTmp4(b);

    b.mmov(static_cast<int>(IR::PC), static_cast<int>(IR::A));
    imm(b, IR::B, 8);
    b.mmov(static_cast<int>(IR::A), static_cast<int>(IR::B), UnitId::ALU, isa::ADD);
    b.mmov(static_cast<int>(IR::ALU_RESULT), static_cast<int>(IR::TMP4));
    pushTmp4(b);

    // R31 contains this call's return address while the callee is running.
    b.mmov(static_cast<int>(IR::TMP4), static_cast<int>(IR::REG_VAL));
    writeArch(b, 31, IR::REG_VAL);

    // PC = instruction address + branch offset.
    b.mmov(static_cast<int>(IR::PC), static_cast<int>(IR::A));
    b.mmov(static_cast<int>(IR::BRANCH_TARGET), static_cast<int>(IR::B));
    b.mmov(static_cast<int>(IR::A), static_cast<int>(IR::B), UnitId::ALU, isa::ADD);
    b.mmov(static_cast<int>(IR::ALU_RESULT), static_cast<int>(IR::PC));

    b.mb("MICROCODE_END");
    b.finishRoutine();
}

} // namespace control

#endif
