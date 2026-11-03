#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Simulator/program_loader.h"
#include "Simulator/cpu.h"

// Assembles `src` and loads it into `cpu`. Returns false (and prints the
// assembler's errors) if assembly failed.
inline bool boot(risc201::CPU& cpu, const std::vector<std::string>& src) {
    std::ostringstream err;
    risc201::AssemblyResult result = risc201::assemble(src, err);
    if (!result.ok()) std::cerr << err.str();
    cpu.load_program(result.words());
    return result.ok();
}

inline int32_t reg(const risc201::CPU& cpu, int n) {
    return cpu.regs().read(static_cast<uint8_t>(n));
}
