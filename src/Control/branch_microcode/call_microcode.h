
#ifndef CALL_MICROCODE_H
#define CALL_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildCallRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("CALL", isa::CALL);

    // 1. Calculate the return address: PC + 8.
    b.mmov(
        static_cast<int>(IR::PC),
        static_cast<int>(IR::A)
    );

    b.append(Microinstruction{
        MicroType::MMOVI,
        -1,
        static_cast<int>(IR::B),
        8
    });

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::ADD
    );

    // 2. Save the return address in architectural R31.
    b.append(Microinstruction{
        MicroType::MMOVI,
        -1,
        static_cast<int>(IR::REG_SRC),
        31
    });

    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::ALU_RESULT),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::WRITE)
    );

    // 3. Calculate the target address: PC + branch offset.
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

    // 4. Update PC to the target.
    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::PC)
    );

    b.mb("MICROCODE_END");
    b.finishRoutine();
}

} // namespace control

#endif
