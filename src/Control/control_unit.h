
#ifndef CONTROL_UNIT_H
#define CONTROL_UNIT_H

#include <array>
#include <cstddef>
#include <cstdint>

#include "microcode.h"
#include "internal_registers.h"
#include "../ALU/alu/alu.h"
#include "../ALU/fpu/fpu.h"
#include "../datapath/register_file.h"
#include "../datapath/memory.h"

namespace control {

using InternalRegisterValues =
    std::array<std::int64_t, INTERNAL_REGISTER_COUNT>;

class ControlUnit { 
public:
    ControlUnit();

    // Load the complete microprogram into this Control Unit.
    void loadProgram(const MicrocodeProgram& program);

    // Load an ISA instruction and its architectural address.
    void loadInstruction(
        std::uint64_t instruction,
        std::uint64_t instructionAddress
    );

    // Execute one microinstruction.
    // Returns false when no instruction is currently being executed.
    bool stepMicroinstruction();

    // Execute the current ISA instruction until its routine completes.
    void executeInstruction();

    // Read the current microprogram counter.
    int getMicroPC() const;

    // Access internal registers.
    std::int64_t readInternal(InternalRegister reg) const;

    void writeInternal(
        InternalRegister reg,
        std::int64_t value
    );

    // Read-only access to Control Memory.
    const MicrocodeProgram& getProgram() const;

    std::int32_t readRegister(std::size_t index) const;
    void writeRegister(std::size_t index, std::int32_t value);

    // Data memory access for instruction setup and testing.
    std::uint32_t readMemoryWord(std::size_t address) const;
    void writeMemoryWord(std::size_t address, std::uint32_t value);


private:
    ALU alu_;
    FPU fpu_;
    MicrocodeProgram program_;
    InternalRegisterValues registers_{};
    risc201::RegisterFile registerFile_;
    memory::Memory memory_;

    int microPC_ = -1;
    bool programLoaded_ = false;
    bool instructionActive_ = false;
    bool pcModified_ = false;
};

} // namespace control

#endif
