
#ifndef XOR_MICROCODE_H
#define XOR_MICROCODE_H

#include "../microcode_builder.h"
#include "../isa_opcodes.h"
#include "../internal_registers.h"

namespace control {

inline void buildXorRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("XOR", isa::XOR);

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

    // Select immediate or register mode.
    b.mbeq(
        static_cast<int>(IR::I),
        1,
        "XOR_IMMEDIATE"
    );

    // Register mode: read rs2 into B.
    b.mmov(
        static_cast<int>(IR::RS2),
        static_cast<int>(IR::REG_SRC)
    );

    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::B),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    b.mb("XOR_EXECUTE");

    // Immediate mode: copy immediate into B.
    b.label("XOR_IMMEDIATE");

    b.mmov(
        static_cast<int>(IR::IMMX),
        static_cast<int>(IR::B)
    );

    // Execute A XOR B.
    b.label("XOR_EXECUTE");

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::ALU,
        isa::XOR
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
