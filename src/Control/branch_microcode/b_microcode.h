
#ifndef B_MICROCODE_H
#define B_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildBRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("B", isa::B);

    // Unconditional branch: PC = PC + branch offset.
    b.mmov(
        static_cast<int>(IR::PC),
        static_cast<int>(IR::A)
    );

    b.mmov(
        static_cast<int>(IR::BRANCH_TARGET),
        static_cast<int>(IR::B)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::ADD
    );

    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::PC)
    );

    b.mb("MICROCODE_END");

    b.finishRoutine();
}

} // namespace control

#endif
