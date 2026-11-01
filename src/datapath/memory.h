#ifndef MEMORY_H
#define MEMORY_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace memory {

// Data memory: 128 KB, big-endian, 4-byte words that must be 4-byte aligned.
constexpr std::size_t kDefaultSize = 131072;

class Memory {
public:
    explicit Memory(std::size_t size = kDefaultSize);

    std::uint8_t readByte(std::size_t address) const;
    void writeByte(std::size_t address, std::uint8_t value);

    std::uint32_t readWord(std::size_t address) const;
    void writeWord(std::size_t address, std::uint32_t value);

    std::size_t size() const;

private:
    std::vector<std::uint8_t> data_;

    void checkAddress(std::size_t address, std::size_t bytes) const;
};

} // namespace memory

#endif