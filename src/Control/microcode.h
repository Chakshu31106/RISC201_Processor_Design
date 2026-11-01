#ifndef MICROCODE_H
#define MICROCODE_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "microinstruction.h"

namespace control {

class MicrocodeProgram {
public:
    // Stores all microinstructions in address order.
    std::vector<Microinstruction> controlMemory;

    // Maps a 7-bit ISA opcode to its routine's starting microaddress.
    // -1 means that no routine has been registered for that opcode.
    static constexpr std::size_t OPCODE_COUNT = 128;

    MicrocodeProgram();

    // Adds a routine to Control Memory and returns its starting address.
    int addRoutine(
        std::uint8_t opcode,
        const std::vector<Microinstruction>& routine
    );

    // Appends instructions without registering an ISA opcode.
    // Useful for shared microcode such as the preamble.
    int appendUnmapped(
        const std::vector<Microinstruction>& routine
    );

    // Returns the routine's starting address, or -1 if it is missing.
    int getEntryAddress(std::uint8_t opcode) const;

    // Returns the number of stored microinstructions.
    std::size_t size() const;

private:
    int opcodeEntry[OPCODE_COUNT];
};

} 

#endif