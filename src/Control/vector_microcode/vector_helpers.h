
#ifndef VECTOR_HELPERS_H
#define VECTOR_HELPERS_H

#include <string>

#include "../microcode_builder.h"
#include "../internal_registers.h"
#include "../isa_opcodes.h"

namespace control {

inline void emitVectorRegisterSelection(
    MicrocodeBuilder& b,
    InternalRegister vectorId,
    int component,
    InternalRegister destination,
    const std::string& prefix
) {
    const int base = 20 + component;

    b.mbeq(static_cast<int>(vectorId), 0, prefix + "_V0");
    b.mbeq(static_cast<int>(vectorId), 1, prefix + "_V1");
    b.mbeq(static_cast<int>(vectorId), 2, prefix + "_V2");

    // Invalid vector ID: end this instruction.
    b.mb("MICROCODE_END");

    b.label(prefix + "_V0");
    b.append(Microinstruction{
        MicroType::MMOVI, -1, static_cast<int>(destination), base
    });
    b.mb(prefix + "_DONE");

    b.label(prefix + "_V1");
    b.append(Microinstruction{
        MicroType::MMOVI, -1, static_cast<int>(destination), base + 3
    });
    b.mb(prefix + "_DONE");

    b.label(prefix + "_V2");
    b.append(Microinstruction{
        MicroType::MMOVI, -1, static_cast<int>(destination), base + 6
    });

    b.label(prefix + "_DONE");
}

inline void emitVectorBinaryRoutine(
    MicrocodeBuilder& b,
    const std::string& name,
    int vectorOpcode,
    int fpuOpcode
) {
    using IR = InternalRegister;

    b.beginRoutine(name, vectorOpcode);

    for (int component = 0; component < 3; ++component) {
        const std::string suffix = "_" + std::to_string(component);

        // Select and read the first source component.
        emitVectorRegisterSelection(
            b, IR::RS1, component, IR::REG_SRC,
            name + "_SRC1" + suffix
        );
        b.mmov(
            static_cast<int>(IR::REG_SRC),
            static_cast<int>(IR::A),
            UnitId::REGISTER_FILE,
            static_cast<int>(RegisterFileAction::READ)
        );

        // Select and read the second source component.
        emitVectorRegisterSelection(
            b, IR::RS2, component, IR::REG_SRC,
            name + "_SRC2" + suffix
        );
        b.mmov(
            static_cast<int>(IR::REG_SRC),
            static_cast<int>(IR::B),
            UnitId::REGISTER_FILE,
            static_cast<int>(RegisterFileAction::READ)
        );

        // Perform the selected floating-point operation.
        b.mmov(
            static_cast<int>(IR::A),
            static_cast<int>(IR::B),
            UnitId::FPU,
            fpuOpcode
        );

        // Select the destination component and store the result.
        emitVectorRegisterSelection(
            b, IR::RD, component, IR::REG_SRC,
            name + "_DEST" + suffix
        );
        b.mmov(
            static_cast<int>(IR::REG_SRC),
            static_cast<int>(IR::ALU_RESULT),
            UnitId::REGISTER_FILE,
            static_cast<int>(RegisterFileAction::WRITE)
        );
    }

    b.mb("MICROCODE_END");
    b.finishRoutine();
}

} // namespace control

#endif
