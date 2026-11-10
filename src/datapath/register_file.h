#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "ISA/registers.h"

namespace risc201 {

// How register / memory words are shown to the user. Every register and memory word is a raw
// 32-bit pattern: integer instructions treat it as an int, float instructions (fadd, vadd, ...)
// treat the SAME bits as an IEEE-754 float. Change with the `fmt` debugger command or --fmt.
enum class NumFormat { Int, Float, Both };

// The one display setting shared by the debugger, trace output and register dump.
inline NumFormat& display_format() {
    static NumFormat f = NumFormat::Both;
    return f;
}

// The 32 bits of `v` read as an IEEE-754 float, as text (e.g. 1065353216 -> "1").
inline std::string float_text(int32_t v) {
    float f;
    std::memcpy(&f, &v, sizeof f);
    char buf[48];
    std::snprintf(buf, sizeof buf, "%.7g", static_cast<double>(f));
    return buf;
}

// `v` in the current display format: "12", "1.5f" or "1069547520 (1.5f)".
inline std::string value_text(int32_t v, NumFormat fmt) {
    if (fmt == NumFormat::Int) return std::to_string(v);
    if (fmt == NumFormat::Float) return float_text(v) + "f";
    return std::to_string(v) + " (" + float_text(v) + "f)";
}
inline std::string value_text(int32_t v) { return value_text(v, display_format()); }

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
    // fmt = Int keeps the classic two-column layout; Float / Both print one register per line.
    std::string dump(NumFormat fmt = NumFormat::Int) const;

private:
    int32_t regs_[NUM_REGS];
    bool e_, gt_, st_;
};

}  // namespace risc201
