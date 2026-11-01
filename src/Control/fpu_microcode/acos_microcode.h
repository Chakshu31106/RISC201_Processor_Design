#ifndef FACOS_MICROCODE_H
#define FACOS_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

// ACOS rd, rs1: R[rd] = acos(R[rs1]) using the FPU's IEEE-754 bit-pattern API.
inline void buildAcosRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("ACOS", isa::ACOS);

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
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::ACOS
    );
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
