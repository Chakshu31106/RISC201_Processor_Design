
#ifndef VANG_MICROCODE_H
#define VANG_MICROCODE_H

#include "vector_helpers.h"
#include "../microinstruction.h"

namespace control {

inline void buildVangRoutine(MicrocodeBuilder& b) {
    using IR = InternalRegister;

    b.beginRoutine("VANG", isa::VANG);

    // 1. Compute dot product: A dot B.
    // Result is preserved in TMP1.
    for (int component = 0; component < 3; ++component) {
        const std::string suffix = "_" + std::to_string(component);

        emitVectorRegisterSelection(
            b, IR::RS1, component, IR::REG_SRC,
            "VANG_DOT_A" + suffix
        );

        b.mmov(
            static_cast<int>(IR::REG_SRC),
            static_cast<int>(IR::A),
            UnitId::REGISTER_FILE,
            static_cast<int>(RegisterFileAction::READ)
        );

        emitVectorRegisterSelection(
            b, IR::RS2, component, IR::REG_SRC,
            "VANG_DOT_B" + suffix
        );

        b.mmov(
            static_cast<int>(IR::REG_SRC),
            static_cast<int>(IR::B),
            UnitId::REGISTER_FILE,
            static_cast<int>(RegisterFileAction::READ)
        );

        b.mmov(
            static_cast<int>(IR::A),
            static_cast<int>(IR::B),
            UnitId::FPU,
            isa::FMUL
        );

        if (component == 0) {
            b.mmov(
                static_cast<int>(IR::ALU_RESULT),
                static_cast<int>(IR::TMP1)
            );
        } else {
            b.mmov(
                static_cast<int>(IR::ALU_RESULT),
                static_cast<int>(IR::B)
            );

            b.mmov(
                static_cast<int>(IR::TMP1),
                static_cast<int>(IR::A)
            );

            b.mmov(
                static_cast<int>(IR::A),
                static_cast<int>(IR::B),
                UnitId::FPU,
                isa::FADD
            );

            b.mmov(
                static_cast<int>(IR::ALU_RESULT),
                static_cast<int>(IR::TMP1)
            );
        }
    }

    // 2. Compute magnitude of a vector:
    // sqrt(x*x + y*y + z*z).
    //
    // The vector field is RS1 or RS2.
    // The result is written to the selected output.
    // TMP4 is the temporary accumulator.
    auto emitMagnitude = [&](InternalRegister vectorField,
                             const std::string& prefix,
                             InternalRegister output) {
        for (int component = 0; component < 3; ++component) {
            const std::string suffix =
                "_" + std::to_string(component);

            emitVectorRegisterSelection(
                b, vectorField, component, IR::REG_SRC,
                prefix + suffix
            );

            b.mmov(
                static_cast<int>(IR::REG_SRC),
                static_cast<int>(IR::A),
                UnitId::REGISTER_FILE,
                static_cast<int>(RegisterFileAction::READ)
            );

            // Square this component.
            b.mmov(
                static_cast<int>(IR::A),
                static_cast<int>(IR::B)
            );

            b.mmov(
                static_cast<int>(IR::A),
                static_cast<int>(IR::B),
                UnitId::FPU,
                isa::FMUL
            );

            if (component == 0) {
                b.mmov(
                    static_cast<int>(IR::ALU_RESULT),
                    static_cast<int>(IR::TMP4)
                );
            } else {
                // Add the square to the accumulated sum.
                b.mmov(
                    static_cast<int>(IR::ALU_RESULT),
                    static_cast<int>(IR::B)
                );

                b.mmov(
                    static_cast<int>(IR::TMP4),
                    static_cast<int>(IR::A)
                );

                b.mmov(
                    static_cast<int>(IR::A),
                    static_cast<int>(IR::B),
                    UnitId::FPU,
                    isa::FADD
                );

                b.mmov(
                    static_cast<int>(IR::ALU_RESULT),
                    static_cast<int>(IR::TMP4)
                );
            }
        }

        // Take sqrt of the accumulated squared components.
        b.mmov(
            static_cast<int>(IR::TMP4),
            static_cast<int>(IR::A)
        );

        b.mmov(
            static_cast<int>(IR::A),
            static_cast<int>(IR::B),
            UnitId::FPU,
            isa::SQRT
        );

        b.mmov(
            static_cast<int>(IR::ALU_RESULT),
            static_cast<int>(output)
        );
    };

    // 3. Calculate |A| and |B|.
    emitMagnitude(IR::RS1, "VANG_MAG_A", IR::TMP2);
    emitMagnitude(IR::RS2, "VANG_MAG_B", IR::TMP3);

    // 4. Denominator = |A| * |B|.
    // Save denominator in TMP2.
    b.mmov(
        static_cast<int>(IR::TMP2),
        static_cast<int>(IR::A)
    );

    b.mmov(
        static_cast<int>(IR::TMP3),
        static_cast<int>(IR::B)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::FMUL
    );

    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::TMP2)
    );

    // 5. Cosine = (A dot B) / (|A| * |B|).
    // Save cosine in TMP3.
    b.mmov(
        static_cast<int>(IR::TMP1),
        static_cast<int>(IR::A)
    );

    b.mmov(
        static_cast<int>(IR::TMP2),
        static_cast<int>(IR::B)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::FDIV
    );

    b.mmov(
        static_cast<int>(IR::ALU_RESULT),
        static_cast<int>(IR::TMP3)
    );

    // 6. Final angle = acos(cosine).
    // This invokes the ACOS dispatch added to control_unit.cpp.
    b.mmov(
        static_cast<int>(IR::TMP3),
        static_cast<int>(IR::A)
    );

    b.mmov(
        static_cast<int>(IR::A),
        static_cast<int>(IR::B),
        UnitId::FPU,
        isa::ACOS
    );

    // 7. Write the angle to architectural register rd.
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
