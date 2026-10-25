#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace risc201 {

// Program storage and the "fetch" half of fetch/decode.
//
// Holds the program as 64-bit words. Every instruction is 8 bytes, so the
// instruction pointer (r29) is a BYTE address and word index = pc / 8.
class InstructionMemory {
public:
    // Loads the assembler's .hex output (see risc201/isa/hex_file.h).
    // Throws std::runtime_error on a missing file or malformed line.
    void load_hex(const std::string& path);

    // Loads words directly (handy for tests).
    void load_words(const std::vector<uint64_t>& words) { words_ = words; }

    // True if `pc` points at a real instruction (aligned and inside the
    // program). The CPU uses this to detect "ran off the end".
    bool in_range(uint32_t pc) const;

    // Returns the instruction word at byte address `pc`.
    // Throws std::runtime_error if pc is not a multiple of 8, and
    // std::out_of_range if pc is past the end of the program.
    uint64_t fetch(uint32_t pc) const;

    size_t size_words() const { return words_.size(); }

private:
    std::vector<uint64_t> words_;
};

}  // namespace risc201
