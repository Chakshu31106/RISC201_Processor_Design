#pragma once

#include <cstdint>

#include "Control/control_unit.h"
#include "datapath/memory.h"
#include "datapath/register_file.h"
#include "Simulator/decoder.h"

namespace risc201 {

// Runs ONE instruction through the microprogrammed ControlUnit, using the
// CPU's own registers, flags and data memory.
//
// The ControlUnit keeps private copies of the registers and memory, so this
// class copies the architectural state in before the instruction and copies
// the results back afterwards. (Memory is copied only for the words the
// instruction reads or writes: ld/st/vld/vst and the stack instructions.) It uses only the ControlUnit's public API.
class MicroExecutor {
public:
    MicroExecutor();

    // True if the microcode has a routine for this instruction's opcode.
    bool can_run(const DecodedInstr& d) const;

    // Executes `word` (already decoded as `d`) located at `pc`. Updates `regs`
    // and `dmem`; returns the next ip. Throws on control-unit errors.
    uint32_t execute(const DecodedInstr& d, uint32_t pc, RegisterFile& regs, memory::Memory& dmem);

private:
    control::ControlUnit cu_;
};

}  // namespace risc201
