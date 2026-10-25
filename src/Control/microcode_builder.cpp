
#include "microcode_builder.h"

#include <stdexcept>
#include <utility>

namespace control {

void MicrocodeBuilder::beginRoutine(
    const std::string& name,
    int opcode
) {
    if (finalized) {
        throw std::logic_error(
            "Cannot add a routine after finalization"
        );
    }

    if (routineOpen) {
        throw std::logic_error(
            "Finish the current routine before starting another"
        );
    }

    if (name.empty()) {
        throw std::invalid_argument(
            "Routine name cannot be empty"
        );
    }

    if (opcode < -1 || opcode >=
        static_cast<int>(MicrocodeProgram::OPCODE_COUNT)) {
        throw std::out_of_range(
            "Opcode must be -1 or between 0 and 127"
        );
    }

    if (labels.find(name) != labels.end()) {
        throw std::invalid_argument(
            "Duplicate microcode label: " + name
        );
    }

    currentRoutineStart = instructions.size();
    routines.push_back({
        name,
        opcode,
        currentRoutineStart,
        currentRoutineStart
    });
    routineOpen = true;

    // A routine's name is also its entry label.
    label(name);
}

void MicrocodeBuilder::label(const std::string& name) {
    if (finalized) {
        throw std::logic_error(
            "Cannot add labels after finalization"
        );
    }

    if (!routineOpen) {
        throw std::logic_error(
            "Labels must be defined inside a routine"
        );
    }

    if (name.empty()) {
        throw std::invalid_argument(
            "Label cannot be empty"
        );
    }

    if (labels.find(name) != labels.end()) {
        throw std::invalid_argument(
            "Duplicate microcode label: " + name
        );
    }

    labels.emplace(name, instructions.size());
}

void MicrocodeBuilder::append(
    const Microinstruction& instruction
) {
    if (finalized) {
        throw std::logic_error(
            "Cannot append instructions after finalization"
        );
    }

    if (!routineOpen) {
        throw std::logic_error(
            "Begin a routine before appending instructions"
        );
    }

    instructions.push_back(instruction);
}


void MicrocodeBuilder::mmov(
    int src,
    int dest,
    UnitId unit,
    int opcode
) {
    Microinstruction instruction{};
    instruction.type = MicroType::MMOV;
    instruction.src = src;
    instruction.dest = dest;
    instruction.unit = unit;
    instruction.opcode = opcode;

    append(instruction);
}


void MicrocodeBuilder::mb(
    const std::string& targetLabel
) {
    if (targetLabel.empty()) {
        throw std::invalid_argument(
            "Jump target label cannot be empty"
        );
    }

    Microinstruction instruction{};
    instruction.type = MicroType::MB;
    instruction.seqAddr = -1;

    const std::size_t instructionIndex = instructions.size();

    append(instruction);

    pendingJumps.push_back({
        instructionIndex,
        targetLabel
    });
}

void MicrocodeBuilder::mbeq(
    int src,
    std::int32_t value,
    const std::string& targetLabel
) {
    if (targetLabel.empty()) {
        throw std::invalid_argument(
            "Jump target label cannot be empty"
        );
    }

    Microinstruction instruction{};
    instruction.type = MicroType::MBEQ;
    instruction.src = src;
    instruction.uimm = value;
    instruction.seqAddr = -1;

    const std::size_t instructionIndex = instructions.size();

    append(instruction);

    pendingJumps.push_back({
        instructionIndex,
        targetLabel
    });
}

void MicrocodeBuilder::finishRoutine() {
    if (finalized) {
        throw std::logic_error(
            "Cannot finish a routine after finalization"
        );
    }

    if (!routineOpen) {
        throw std::logic_error(
            "No routine is currently open"
        );
    }

    if (instructions.size() == currentRoutineStart) {
        throw std::invalid_argument(
            "A microcode routine cannot be empty"
        );
    }

    // The opcode is supplied to beginRoutine().
    // Recover it from the routine's entry metadata below.
    // The actual routine record is created in beginRoutine().
    routines.back().end = instructions.size();

    routineOpen = false;
}

void MicrocodeBuilder::finalize(
    MicrocodeProgram& program
) {
    if (finalized) {
        throw std::logic_error(
            "Microcode program has already been finalized"
        );
    }

    if (routineOpen) {
        throw std::logic_error(
            "Finish the current routine before finalization"
        );
    }

    if (routines.empty()) {
        throw std::invalid_argument(
            "Cannot finalize an empty microcode program"
        );
    }

    if (program.size() != 0) {
        throw std::invalid_argument(
            "Finalize into an empty MicrocodeProgram"
        );
    }

    // Check that every routine has a valid range and that ISA
    // opcodes are not registered more than once.
    for (std::size_t i = 0; i < routines.size(); ++i) {
        const Routine& routine = routines[i];

        if (routine.start >= routine.end ||
            routine.end > instructions.size()) {
            throw std::logic_error(
                "Invalid instruction range for routine: " +
                routine.name
            );
        }

        if (i > 0 && routines[i - 1].end != routine.start) {
            throw std::logic_error(
                "Microcode routines are not contiguous"
            );
        }

        if (routine.opcode >= 0) {
            for (std::size_t j = 0; j < i; ++j) {
                if (routines[j].opcode == routine.opcode) {
                    throw std::invalid_argument(
                        "Duplicate opcode registration"
                    );
                }
            }
        }
    }

    // Resolve all symbolic jumps against the global label table.
    std::vector<Microinstruction> resolved = instructions;

    for (const PendingJump& jump : pendingJumps) {
        const auto it = labels.find(jump.targetLabel);

        if (it == labels.end()) {
            throw std::invalid_argument(
                "Undefined microcode label: " +
                jump.targetLabel
            );
        }

        // A jump must target an actual instruction, not the
        // position immediately after the last instruction.
        if (it->second >= resolved.size()) {
            throw std::out_of_range(
                "Microcode label does not point to an instruction: " +
                jump.targetLabel
            );
        }

        resolved[jump.instructionIndex].seqAddr =
            static_cast<int>(it->second);
    }

    // Append routines in exactly the order in which they were built.
    // Therefore, their global label addresses remain valid.
    for (const Routine& routine : routines) {
        std::vector<Microinstruction> segment(
            resolved.begin() +
                static_cast<std::ptrdiff_t>(routine.start),
            resolved.begin() +
                static_cast<std::ptrdiff_t>(routine.end)
        );

        int actualStart;

        if (routine.opcode == -1) {
            actualStart = program.appendUnmapped(segment);
        } else {
            actualStart = program.addRoutine(
                static_cast<std::uint8_t>(routine.opcode),
                segment
            );
        }

        if (actualStart !=
            static_cast<int>(routine.start)) {
            throw std::logic_error(
                "Control Memory address mismatch"
            );
        }
    }

    finalized = true;
}

} // namespace control
