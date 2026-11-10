
#ifndef ABS_MICROCODE_H
#define ABS_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildAbsRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("ABS", isa::ABS);

    // 0. abs uses CMP internally to test the sign. Only cmp/fcmp may change the architectural
    //    flags, so save E/GT/ST here and put them back before the routine ends.
    b.mmov(static_cast<int>(IR::FLAG_E),  static_cast<int>(IR::TMP2));
    b.mmov(static_cast<int>(IR::FLAG_GT), static_cast<int>(IR::TMP3));
    b.mmov(static_cast<int>(IR::FLAG_ST), static_cast<int>(IR::TMP4));

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

    // 9. Write the result back to rs1. abs is a one-operand (R1S) instruction like incr/decr:
    //    the decoder leaves rd at 0 and the register to update is in rs1.
    b.label("ABS_WRITEBACK");

    b.mmov(
        static_cast<int>(IR::RS1),
        static_cast<int>(IR::REG_SRC)
    );

    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::ALU_RESULT),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::WRITE)
    );

    // 10. Restore the flags saved at the start.
    b.mmov(static_cast<int>(IR::TMP2), static_cast<int>(IR::FLAG_E));
    b.mmov(static_cast<int>(IR::TMP3), static_cast<int>(IR::FLAG_GT));
    b.mmov(static_cast<int>(IR::TMP4), static_cast<int>(IR::FLAG_ST));

    // 11. Return to the shared preamble.
    b.mb("MICROCODE_END");

    b.finishRoutine();
}

} // namespace control

#endif
