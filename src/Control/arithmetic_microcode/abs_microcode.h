
#ifndef ABS_MICROCODE_H
#define ABS_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildAbsRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("ABS", isa::ABS);

    // 1. Select rs1 and read its value.
    b.mmov(
        static_cast<int>(IR::RS1),
        static_cast<int>(IR::REG_SRC)
    );

    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::REG_VAL),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    // 2. Preserve the original value.
    b.mmov(
        static_cast<int>(IR::REG_VAL),
        static_cast<int>(IR::TMP1)
    );

    // 3. Set A = original value.
    b.mmov(
        static_cast<int>(IR::TMP1),
        static_cast<int>(IR::A)
    );

    // 4. Set B = 0 using MMOVI.
    b.append(Microinstruction{
        MicroType::MMOVI,
        -1,
        static_cast<int>(IR::B),
        0
    });

    // 5. Compare A with B (zero).
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::CMP
    );

    // 6. If FLAG_ST == 1, the original value is negative.
    b.mbeq(
        static_cast<int>(IR::FLAG_ST),
        1,
        "ABS_NEG"
    );

    // 7. Non-negative: result is the original value.
    b.mmov(
        static_cast<int>(IR::TMP1),
        static_cast<int>(IR::ALU_RESULT)
    );

    b.mb("ABS_WRITEBACK");

    // 8. Negative: calculate 0 - original value.
    b.label("ABS_NEG");

    b.append(Microinstruction{
        MicroType::MMOVI,
        -1,
        static_cast<int>(IR::A),
        0
    });

    b.mmov(
        static_cast<int>(IR::TMP1),
        static_cast<int>(IR::B)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::SUB
    );

    // 9. Write the result to architectural destination rd.
    b.label("ABS_WRITEBACK");

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

    // 10. Return to the shared preamble.
    b.mb("MICROCODE_END");

    b.finishRoutine();
}

} // namespace control

#endif
