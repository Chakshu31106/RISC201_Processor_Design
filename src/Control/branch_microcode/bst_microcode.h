
#ifndef BST_MICROCODE_H
#define BST_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildBstRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("BST", isa::BST);

    // If FLAG_ST == 1, the branch is taken.
    b.mbeq(
        static_cast<int>(IR::FLAG_ST),
        1,
        "BST_TAKEN"
    );

    // Otherwise, end the BST routine.
    b.mb("MICROCODE_END");

    // Taken branch: PC = PC + branch offset.
    b.label("BST_TAKEN");

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
