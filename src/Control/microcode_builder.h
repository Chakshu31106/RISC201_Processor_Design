
#ifndef MICROCODE_BUILDER_H
#define MICROCODE_BUILDER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "microcode.h"

namespace control {

class MicrocodeBuilder {
public:
    // Begin a routine.
    // Use opcode = -1 for a routine such as the shared preamble
    // that has no ISA opcode of its own.
    void beginRoutine(const std::string& name, int opcode);

    // Define a label at the current global microinstruction address.
    void label(const std::string& name);

    // Append a microinstruction to the current routine.
    void append(const Microinstruction& instruction);

    // Append a move or functional-unit operation.
    void mmov(
        int src,
        int dest,
        UnitId unit = UnitId::NONE,
        int opcode = -1
    );

    // Append an unconditional microcode jump to a named label.
    void mb(const std::string& targetLabel);

    // Compare src with value; jump if equal.
    void mbeq(
        int src,
        std::int32_t value,
        const std::string& targetLabel
    );

    // Finish the current routine.
    void finishRoutine();

    // Resolve every label and load the complete program into Control Memory.
    void finalize(MicrocodeProgram& program);

private:
    struct Routine {
        std::string name;
        int opcode;
        std::size_t start;
        std::size_t end;
    };

    struct PendingJump {
        std::size_t instructionIndex;
        std::string targetLabel;
    };

    std::vector<Microinstruction> instructions;
    std::vector<Routine> routines;
    std::unordered_map<std::string, std::size_t> labels;
    std::vector<PendingJump> pendingJumps;

    bool routineOpen = false;
    bool finalized = false;
    std::size_t currentRoutineStart = 0;
};

} // namespace control

#endif
