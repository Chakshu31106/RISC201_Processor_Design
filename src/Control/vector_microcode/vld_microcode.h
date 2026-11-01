
#ifndef VLD_MICROCODE_H
#define VLD_MICROCODE_H

#include "vector_helpers.h"
#include "../microinstruction.h"

namespace control {

inline void buildVldRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("VLD", isa::VLD);

    // Step 1: Read the scalar base register R[rs1].
    b.mmov(
        static_cast<int>(IR::RS1),
        static_cast<int>(IR::REG_SRC),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    // Step 2: Save the base address in MAR.
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::MAR)
    );

    // Step 3: Load the signed immediate offset.
    b.mmov(
        static_cast<int>(IR::IMMX),
        static_cast<int>(IR::REG_SRC)
    );

    // Step 4: Calculate the first component address.
    // MAR = R[rs1] + offset.
    b.mmov(
        static_cast<int>(IR::MAR),
        static_cast<int>(IR::REG_SRC),
        UnitId::ALU,
        isa::ADD
    );

    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::MAR)
    );

    // Load X, Y, and Z from three consecutive 32-bit words.
    for (int component = 0; component < 3; ++component) {
        // Read memory[MAR] into MDR.
        b.mmov(
            static_cast<int>(IR::MAR),
            static_cast<int>(IR::MDR),
            UnitId::MEMORY,
            static_cast<int>(MemoryAction::READ_WORD)
        );

        // Select destination vector component.
        emitVectorRegisterSelection(
            b,
            IR::RD,
            component,
            IR::REG_SRC,
            "VLD_DEST_" + std::to_string(component)
        );

        // Preserve loaded data for the register-file write.
        b.mmov(
            static_cast<int>(IR::MDR),
            static_cast<int>(IR::REG_VAL)
        );

        // Write the loaded component into the selected register.
        b.mmov(
            static_cast<int>(IR::REG_SRC),
            static_cast<int>(IR::REG_VAL),
            UnitId::REGISTER_FILE,
            static_cast<int>(RegisterFileAction::WRITE)
        );

        // Advance to the next 32-bit component.
        if (component < 2) {
            b.append(Microinstruction{
                MicroType::MMOVI,
                -1,
                static_cast<int>(IR::TMP1),
                4
            });

            b.mmov(
                static_cast<int>(IR::MAR),
                static_cast<int>(IR::TMP1),
                UnitId::ALU,
                isa::ADD
            );

            b.mmov(
                static_cast<int>(IR::ALU_RESULT),
                static_cast<int>(IR::MAR)
            );
        }
    }

    b.mb("MICROCODE_END");
    b.finishRoutine();
}

} // namespace control

#endif
