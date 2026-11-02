#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

// Assembles source text into a program the CPU can load, using the core in
// assembler/risc201_common.h (tokenize -> pass 1 -> encode).

namespace risc201 {

// One successfully assembled instruction.
struct AssembledInstruction {
    uint32_t address;   // byte address of the instruction
    int line_number;    // 1-based source line
    uint64_t word;      // encoded 64-bit instruction
};

struct AssemblyResult {
    std::vector<AssembledInstruction> instructions;
    int error_count = 0;

    bool ok() const { return error_count == 0; }

    // Just the encoded words, in program order (what goes into a .hex file).
    std::vector<uint64_t> words() const;
};

// Runs the whole pipeline: tokenize -> pass 1 (addresses, symbol table) ->
// pass 2 (encode). Every bad line is reported to `err` as
// "Line N: error: ..." and assembly continues past it, so one run reports all
// errors. If result.ok() is false the program must not be used.
AssemblyResult assemble(const std::vector<std::string>& source, std::ostream& err);

// Reads a file into lines. Throws std::runtime_error if it cannot be opened.
std::vector<std::string> read_source_lines(const std::string& path);

// read_source_lines() + assemble(). Throws std::runtime_error if the file
// cannot be opened; assembly errors are reported through `err` as above.
AssemblyResult assemble_file(const std::string& path, std::ostream& err);

}  // namespace risc201
