
#ifndef DECODED_REGISTERS_H
#define DECODED_REGISTERS_H

#include <array>
#include <cstdint>

#include "instruction_decoder.h"
#include "internal_registers.h"

namespace control {

using InternalRegisterValues =
    std::array<std::int64_t, INTERNAL_REGISTER_COUNT>;

void loadDecodedRegisters(
    std::uint64_t instruction,
    InternalRegisterValues& registers
);

} // namespace control

#endif
