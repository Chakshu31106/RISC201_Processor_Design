#pragma once

#include <cstdint>
#include <string>

#include "isa/registers.h"

namespace risc201 {

// RISC201 register file.
//   r0  - r19 : general purpose
//   r20 - r28 : vector registers (vec0x/y/z, vec1x/y/z, vec2x/y/z)
//   r29       : instruction pointer (ip)
//   r30       : stack pointer (sp)
//   r31       : return address (ra)
// Plus three flag registers: E (beq), GT (bgt), ST (bst).
//
// All registers are 32 bits. Out-of-range register numbers throw
// std::out_of_range.
class RegisterFile {
public:
    static constexpr int NUM_REGS = kNumRegisters;
    static constexpr int IP = kRegIp;
    static constexpr int SP = kRegSp;
    static constexpr int RA = kRegRa;

    RegisterFile();

    // Zeroes every register and clears all flags.
    void reset();

    // Generic access by register number (0-31).
    int32_t read(uint8_t index) const;
    void    write(uint8_t index, int32_t value);

    // Convenience accessors for the special registers.
    int32_t ip() const { return regs_[IP]; }
    int32_t sp() const { return regs_[SP]; }
    int32_t ra() const { return regs_[RA]; }
    void set_ip(int32_t v) { regs_[IP] = v; }
    void set_sp(int32_t v) { regs_[SP] = v; }
    void set_ra(int32_t v) { regs_[RA] = v; }

    // Flag registers. Set together after a cmp / fcmp.
    void set_flags(bool e, bool gt, bool st);
    bool flag_e()  const { return e_; }
    bool flag_gt() const { return gt_; }
    bool flag_st() const { return st_; }

    // Human-readable dump for the CLI / debugger ("regs").
    std::string dump() const;

private:
    int32_t regs_[NUM_REGS];
    bool e_, gt_, st_;
};

}  // namespace risc201
