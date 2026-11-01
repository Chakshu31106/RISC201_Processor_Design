
#ifndef EXECUTION_UNITS_H
#define EXECUTION_UNITS_H

#include <cstdint>

namespace control {

struct ALUResult {
    std::int64_t value = 0;
    bool flagE = false; 
    bool flagGT = false;
    bool flagST = false;
};

// Executes one integer ALU operation.
// The opcode uses the ISA opcode values from isa_opcodes.h.
ALUResult executeALU(
    std::uint8_t opcode,
    std::int64_t operandA,
    std::int64_t operandB
);

} // namespace control

#endif
