#include "datapath/register_file.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace risc201 {

RegisterFile::RegisterFile() { reset(); }

void RegisterFile::reset() {
    for (int i = 0; i < NUM_REGS; ++i) regs_[i] = 0;
    e_ = gt_ = st_ = false;
}

int32_t RegisterFile::read(uint8_t index) const {
    if (index >= NUM_REGS)
        throw std::out_of_range("Register index out of range: " + std::to_string(index));
    return regs_[index];
}

void RegisterFile::write(uint8_t index, int32_t value) {
    if (index >= NUM_REGS)
        throw std::out_of_range("Register index out of range: " + std::to_string(index));
    regs_[index] = value;
}

void RegisterFile::set_flags(bool e, bool gt, bool st) {
    e_ = e;
    gt_ = gt;
    st_ = st;
}

std::string RegisterFile::dump() const {
    std::ostringstream oss;
    for (int i = 0; i < NUM_REGS; ++i) {
        std::string name = "r" + std::to_string(i);
        if (i == IP) name += "(ip)";
        else if (i == SP) name += "(sp)";
        else if (i == RA) name += "(ra)";
        oss << std::left << std::setw(8) << name << "= "
            << std::right << std::setw(11) << regs_[i]
            << "  0x" << std::hex << std::setw(8) << std::setfill('0')
            << static_cast<uint32_t>(regs_[i]) << std::dec << std::setfill(' ');
        oss << ((i % 2 == 1) ? "\n" : "    ");
    }
    oss << "flags: E=" << e_ << " GT=" << gt_ << " ST=" << st_ << "\n";
    return oss.str();
}

}  // namespace risc201
