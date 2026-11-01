
#ifndef CBIT_MICROCODE_H
#define CBIT_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildCbitRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("CBIT", isa::CBIT);

    // 1. Read rs1 into A.
    b.mmov(
        static_cast<int>(IR::RS1),
        static_cast<int>(IR::REG_SRC)
    );

    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::A),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    // 2. Preserve the original value in TMP2.
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::TMP2)
    );

    // 3. Load bit index into B.
    b.mmov(
        static_cast<int>(IR::RS2),
        static_cast<int>(IR::B)
    );

    // 4. Set A = 1.
    b.append(Microinstruction{
        MicroType::MMOVI,
        -1,
        static_cast<int>(IR::A),
        1
    });

    // 5. Calculate mask = 1 << bit_index.
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::LSL
    );

    // 6. Save the mask in TMP1.
    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::TMP1)
    );

    // 7. Load the mask into A for NOT.
    b.mmov(
        static_cast<int>(IR::TMP1),
        static_cast<int>(IR::A)
    );

    // 8. Invert the mask: A = ~mask.
    // Uses the existing NOT operation.
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::NOT
    );

    // 9. Preserve the inverted mask.
    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::TMP1)
    );

    // 10. Restore the original value into A.
    b.mmov(
        static_cast<int>(IR::TMP2),
        static_cast<int>(IR::A)
    );

    // 11. Load the inverted mask into B.
    b.mmov(
        static_cast<int>(IR::TMP1),
        static_cast<int>(IR::B)
    );

    // 12. Calculate result = original_value AND inverted_mask.
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::AND
    );

    // 13. Write ALU_RESULT to rd.
    b.mmov(
        static_cast<int>(IR::RD),
        static_cast<int>(IR::REG_SRC)
    );

    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::ALU_RESULT),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::WRITE)
    );

    b.mb("MICROCODE_END");
    b.finishRoutine();
}

} // namespace control

#endif
