
#ifndef PCNT_MICROCODE_H
#define PCNT_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildPcntRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("PCNT", isa::PCNT);

    // Read rs1 into A.
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

    // Execute population count.
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::PCNT
    );

    // Write result to rd.
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
