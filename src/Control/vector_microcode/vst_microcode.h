
#ifndef VST_MICROCODE_H
#define VST_MICROCODE_H

#include "vector_helpers.h"
#include "../microinstruction.h"

namespace control {

inline void buildVstRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("VST", isa::VST);

    // Step 1: Read the base address from architectural R[rd].
    b.mmov(
        static_cast<int>(IR::RD),
        static_cast<int>(IR::REG_SRC),
        UnitId::REGISTER_FILE,
        static_cast<int>(RegisterFileAction::READ)
    );

    // Step 2: Save the base address in MAR.
    b.mmov(
        static_cast<int>(IR::REG_SRC),
        static_cast<int>(IR::MAR)
    );

    // Step 3: Load the sign-extended immediate offset.
    b.mmov(
        static_cast<int>(IR::IMMX),
        static_cast<int>(IR::REG_SRC)
    );

    // Step 4: Calculate effective address = R[rd] + offset.
    b.mmov(
        static_cast<int>(IR::MAR),
        static_cast<int>(IR::REG_SRC),
        UnitId::ALU,
        isa::ADD
    );

    // Step 5: Save the effective address in MAR.
    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::MAR)
    );

    // Store X, Y, and Z at consecutive word addresses.
    for (int component = 0; component < 3; ++component) {
        // Select the architectural register for this component
        // of the source vector encoded in rs1.
        emitVectorRegisterSelection(
            b,
            IR::RS1,
            component,
            IR::REG_SRC,
            "VST_SRC_" + std::to_string(component)
        );

        // Read the selected vector component.
        b.mmov(
            static_cast<int>(IR::REG_SRC),
            static_cast<int>(IR::MDR),
            UnitId::REGISTER_FILE,
            static_cast<int>(RegisterFileAction::READ)
        );

        // Store MDR at memory[MAR].
        b.mmov(
            static_cast<int>(IR::MAR),
            static_cast<int>(IR::MDR),
            UnitId::MEMORY,
            static_cast<int>(MemoryAction::WRITE_WORD)
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
