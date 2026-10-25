
#ifndef TOG_MICROCODE_H
#define TOG_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildTogRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("TOG", isa::TOG);

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

    // 2. Preserve the original value.
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::TMP2)
    );

    // 3. Load the bit index from RS2 into B.
    b.mmov(
        static_cast<int>(IR::RS2),
        static_cast<int>(IR::B)
    );

    // 4. Set A = 1 to construct the mask.
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

    // 6. Save the mask.
    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::TMP1)
    );

    // 7. Restore the original source value into A.
    b.mmov(
        static_cast<int>(IR::TMP2),
        static_cast<int>(IR::A)
    );

    // 8. Load the mask into B.
    b.mmov(
        static_cast<int>(IR::TMP1),
        static_cast<int>(IR::B)
    );

    // 9. Toggle the selected bit.
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::XOR
    );

    // 10. Write ALU_RESULT to rd.
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
