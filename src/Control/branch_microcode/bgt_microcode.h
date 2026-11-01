
#ifndef BGT_MICROCODE_H
#define BGT_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildBgtRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("BGT", isa::BGT);

    // If FLAG_GT == 1, the branch is taken.
    b.mbeq(
        static_cast<int>(IR::FLAG_GT),
        1,
        "BGT_TAKEN"
    );

    // Otherwise, end the BGT routine.
    b.mb("MICROCODE_END");

    // Taken branch: PC = PC + branch offset.
    b.label("BGT_TAKEN");

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
