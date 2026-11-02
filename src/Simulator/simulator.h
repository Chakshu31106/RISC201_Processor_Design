#pragma once

#include <iosfwd>
#include <string>

#include "Simulator/cpu.h"

namespace risc201 {

// Loads a program into the CPU. A path ending in ".asm" is assembled first
// (assembly errors are reported to `err`); anything else is read as the
// assembler's .hex output. Returns false on any failure.
bool load_program_file(CPU& cpu, const std::string& path, std::ostream& err);

// Draws the stack as ASCII art (used by the "stack" debugger command).
void show_stack(const CPU& cpu, std::ostream& out, uint32_t max_words = 16);

// Interactive debugger loop. Reads commands from `in` until "quit" or EOF.
// Type "help" for the command list.
void run_cli(CPU& cpu, std::istream& in, std::ostream& out);

}  // namespace risc201
