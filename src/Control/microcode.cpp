#include "microcode.h"

#include <stdexcept>

namespace control {

MicrocodeProgram::MicrocodeProgram()
    : controlMemory(),
      opcodeEntry{} {

    // Mark every opcode as having no registered routine.
    for (std::size_t i = 0; i < OPCODE_COUNT; ++i) {
        opcodeEntry[i] = -1;
    }
}

int MicrocodeProgram::addRoutine(
    std::uint8_t opcode,
    const std::vector<Microinstruction>& routine
) {
    // ISA opcodes are 7-bit values: 0 through 127.
    if (opcode >= OPCODE_COUNT) {
        throw std::out_of_range("ISA opcode is not valid");
    }

    if (routine.empty()) {
        throw std::invalid_argument(
            "a microcode routine cannot be empty"
        );
    }

    if (opcodeEntry[opcode] != -1) {
        throw std::invalid_argument(
            "a routine is already registered for this opcode"
        );
    }

    // The current size is the first address of the new routine.
    const std::size_t startAddress = controlMemory.size();

    // Ensure the starting address fits in the int-based address fields.
    if (startAddress >
        static_cast<std::size_t>(__INT_MAX__)) {
        throw std::length_error(
            "control memory exceeds the supported address range"
        );
    }

    // Append the routine to Control Memory.
    controlMemory.insert(
        controlMemory.end(),
        routine.begin(),
        routine.end()
    );

    // Record where this ISA opcode's routine begins.
    opcodeEntry[opcode] = static_cast<int>(startAddress);

    return static_cast<int>(startAddress);
}

int MicrocodeProgram::appendUnmapped(
    const std::vector<Microinstruction>& routine
) {
    if (routine.empty()) {
        throw std::invalid_argument(
            "An unmapped microcode routine cannot be empty"
        );
    }

    const std::size_t startAddress = controlMemory.size();

    const std::size_t maxAddress =
        static_cast<std::size_t>(
            std::numeric_limits<int>::max()
        );

    if (startAddress > maxAddress ||
        routine.size() > maxAddress - startAddress) {
        throw std::length_error(
            "Control Memory exceeds supported address range"
        );
    }

    controlMemory.insert(
        controlMemory.end(),
        routine.begin(),
        routine.end()
    );

    return static_cast<int>(startAddress);
}



int MicrocodeProgram::getEntryAddress(
    std::uint8_t opcode
) const {
    if (opcode >= OPCODE_COUNT) {
        return -1;
    }

    return opcodeEntry[opcode];
}

std::size_t MicrocodeProgram::size() const {
    return controlMemory.size();
}

} // namespace control