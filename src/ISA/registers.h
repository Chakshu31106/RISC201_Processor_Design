#pragma once

#include <cstdint>

// Architectural register numbering from the RISC201 ISA PDF.
namespace risc201 {
inline constexpr int kNumRegisters = 32;
inline constexpr std::uint8_t kRegIp = 29;
inline constexpr std::uint8_t kRegSp = 30;
inline constexpr std::uint8_t kRegRa = 31;
}
