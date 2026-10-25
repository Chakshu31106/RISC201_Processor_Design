
#ifndef RET_MICROCODE_H
#define RET_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildRetRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("RET", isa::RET);

    // 1. Select architectural register R31.
    b.append(Microinstruction{
        MicroType::MMOVI,
        -1,
        static_cast<int>(IR::REG_SRC),
        31
    });

    // 2. Read R31 into A.
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::A),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    // 3. Restore the saved return address.
    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::PC)
    );

    // 4. Return to the normal microcode dispatch.
    b.mb("MICROCODE_END");
    b.finishRoutine();
}

} // namespace control

#endif
