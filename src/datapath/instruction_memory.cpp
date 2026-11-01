#include "datapath/instruction_memory.h"

#include <stdexcept>

#include "isa/hex_file.h"

namespace risc201 {

void InstructionMemory::load_hex(const std::string& path) {
    words_ = read_hex_file(path);
}

bool InstructionMemory::in_range(uint32_t pc) const {
    return pc % 8 == 0 && (pc / 8) < words_.size();
}

uint64_t InstructionMemory::fetch(uint32_t pc) const {
    if (pc % 8 != 0)
        throw std::runtime_error("Misaligned instruction fetch at address " + std::to_string(pc));
    size_t idx = pc / 8;
    if (idx >= words_.size())
        throw std::out_of_range("Instruction fetch past end of program at address " + std::to_string(pc));
    return words_[idx];
}

}  // namespace risc201
