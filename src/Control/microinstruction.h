#ifndef MICROINSTRUCTION_H
#define MICROINSTRUCTION_H

#include <cstdint>

namespace control {

// the 8 microinstruction types in our current design.
enum class MicroType {
    MLOAD_IR,
    MDECODE,
    MSWITCH,
    MMOV,
    MMOVI,
    MADD,
    MB,
    MBEQ
};

// identifies which functional unit a microinstruction commands.
enum class UnitId {
    NONE,
    ALU,
    REGISTER_FILE,
    MEMORY,
    FPU,
    VECTOR
};


enum class RegisterFileAction : int {
    READ = 0,
    WRITE = 1
};

enum class MemoryAction : int {
    READ_WORD = 0,
    WRITE_WORD = 1
};


struct Microinstruction {
    MicroType type = MicroType::MMOV;

    // indices of internal CU registers.
    int src = -1;
    int dest = -1;

    // immediate value and microcode jump target.
    std::int32_t uimm = 0;
    int seqAddr = -1;

    // functional unit and its operation code.
    UnitId unit = UnitId::NONE;
    int opcode = -1;
};



} 

#endif
