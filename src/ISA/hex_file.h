#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace risc201 {

// The .hex program format shared by the assembler (writer) and the simulator
// (reader): one 64-bit instruction word per line, written as 16 lowercase hex
// digits, no "0x" prefix, no addresses, in program order.

// Throws std::runtime_error if the file cannot be written.
void write_hex_file(const std::string& path, const std::vector<uint64_t>& words);

// Companion text format: one 64-character binary string per line. Written next
// to the .hex file by the assembler (same name, ".bin" extension).
void write_bin_file(const std::string& path, const std::vector<uint64_t>& words);
std::string bin_path_for(std::string hex_path);

// Blank lines and ";" comments are skipped. Each line may be 1-16 hex digits
// (optional 0x prefix) or exactly 64 binary digits, so .hex and .bin files
// both load. Throws std::runtime_error if the file cannot be
// opened or a line is not a valid hex word (the message names file and line).
std::vector<uint64_t> read_hex_file(const std::string& path);

}  // namespace risc201
