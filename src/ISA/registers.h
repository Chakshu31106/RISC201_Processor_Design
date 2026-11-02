#pragma once

#include <cstdint>
#include <string>

// Architectural register numbering from the RISC201 ISA PDF.
namespace risc201 {
inline constexpr int kNumRegisters = 32;
inline constexpr std::uint8_t kRegIp = 29;
inline constexpr std::uint8_t kRegSp = 30;
inline constexpr std::uint8_t kRegRa = 31;

// Formats a register number as "rN" text (used by the disassembler and CLI).
std::string register_name(std::uint8_t num);
}
