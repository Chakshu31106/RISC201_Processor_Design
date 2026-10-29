#ifndef STACK_MICROCODE_HELPERS_H
#define STACK_MICROCODE_HELPERS_H

#include "microcode_builder.h"
#include "isa_opcodes.h"
#include "internal_registers.h"
#include "microinstruction.h"

namespace control {
namespace stack_detail {

inline void imm(MicrocodeBuilder& b, InternalRegister dest, std::int32_t value) {
    b.append(Microinstruction{MicroType::MMOVI, -1, static_cast<int>(dest), value});
}

inline void readArch(MicrocodeBuilder& b, int archIndex, InternalRegister dest) {
    imm(b, InternalRegister::REG_SRC, archIndex);
    b.mmov(static_cast<int>(InternalRegister::REG_SRC), static_cast<int>(dest),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::READ));
}

inline void writeArch(MicrocodeBuilder& b, int archIndex, InternalRegister value) {
    imm(b, InternalRegister::REG_SRC, archIndex);
    b.mmov(static_cast<int>(InternalRegister::REG_SRC), static_cast<int>(value),
           UnitId::REGISTER_FILE, static_cast<int>(RegisterFileAction::WRITE));
}

// The stack grows down. R30 points to the next free byte; each item is 4 bytes.
// Push the value held in TMP4, preserving it while R30 is updated.
inline void pushTmp4(MicrocodeBuilder& b) {
    readArch(b, 30, InternalRegister::TMP1);               // old SP
    imm(b, InternalRegister::B, 4);
    b.mmov(static_cast<int>(InternalRegister::TMP1), static_cast<int>(InternalRegister::B), UnitId::ALU, isa::SUB);
    b.mmov(static_cast<int>(InternalRegister::ALU_RESULT), static_cast<int>(InternalRegister::TMP2));
    b.mmov(static_cast<int>(InternalRegister::TMP2), static_cast<int>(InternalRegister::TMP4),
           UnitId::MEMORY, static_cast<int>(MemoryAction::WRITE_WORD));
    b.mmov(static_cast<int>(InternalRegister::TMP2), static_cast<int>(InternalRegister::REG_VAL));
    writeArch(b, 30, InternalRegister::REG_VAL);
}

// Pop the top value into TMP4, then advance R30. Memory bounds checks detect underflow.
inline void popTmp4(MicrocodeBuilder& b) {
    readArch(b, 30, InternalRegister::TMP1);
    b.mmov(static_cast<int>(InternalRegister::TMP1), static_cast<int>(InternalRegister::TMP4),
           UnitId::MEMORY, static_cast<int>(MemoryAction::READ_WORD));
    imm(b, InternalRegister::B, 4);
    b.mmov(static_cast<int>(InternalRegister::TMP1), static_cast<int>(InternalRegister::B), UnitId::ALU, isa::ADD);
    b.mmov(static_cast<int>(InternalRegister::ALU_RESULT), static_cast<int>(InternalRegister::REG_VAL));
    writeArch(b, 30, InternalRegister::REG_VAL);
}

} // namespace stack_detail

} // namespace control

#endif
