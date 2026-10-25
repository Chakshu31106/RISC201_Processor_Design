#include "datapath/memory.h"

#include <stdexcept>

namespace memory {

Memory::Memory(std::size_t size)
    : data_(size, 0) {}

void Memory::checkAddress(
    std::size_t address,
    std::size_t bytes
) const {
    if (address > data_.size() ||
        bytes > data_.size() - address) {
        throw std::out_of_range("Memory address out of range");
    }
}

std::uint8_t Memory::readByte(std::size_t address) const {
    checkAddress(address, 1);
    return data_[address];
}

void Memory::writeByte(
    std::size_t address,
    std::uint8_t value
) {
    checkAddress(address, 1);
    data_[address] = value;
}

std::uint32_t Memory::readWord(std::size_t address) const {
    checkAddress(address, 4);
    if (address % 4 != 0)
        throw std::invalid_argument("Misaligned word read (address must be a multiple of 4)");

    // Big-endian: lowest-addressed byte is most significant.
    return (static_cast<std::uint32_t>(data_[address]) << 24)
         | (static_cast<std::uint32_t>(data_[address + 1]) << 16)
         | (static_cast<std::uint32_t>(data_[address + 2]) << 8)
         |  static_cast<std::uint32_t>(data_[address + 3]);
}

void Memory::writeWord(
    std::size_t address,
    std::uint32_t value
) {
    checkAddress(address, 4);
    if (address % 4 != 0)
        throw std::invalid_argument("Misaligned word write (address must be a multiple of 4)");

    data_[address]     = static_cast<std::uint8_t>(value >> 24);
    data_[address + 1] = static_cast<std::uint8_t>(value >> 16);
    data_[address + 2] = static_cast<std::uint8_t>(value >> 8);
    data_[address + 3] = static_cast<std::uint8_t>(value);
}

std::size_t Memory::size() const {
    return data_.size();
}

} // namespace memory