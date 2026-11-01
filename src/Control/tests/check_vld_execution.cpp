
#include <cassert>
#include <cstdint>
#include <iostream>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

namespace {

constexpr std::uint64_t IMMEDIATE_MODE_BIT = std::uint64_t{1} << 41;
constexpr std::uint64_t IMMEDIATE_MASK =
    (std::uint64_t{1} << 41) - 1;

std::uint64_t encodeVld(
    unsigned vd,
    unsigned rs1,
    std::int64_t offset
) {
    assert(vd < 3);
    assert(rs1 < 32);

    return (std::uint64_t{control::isa::VLD} << 57)
         | (std::uint64_t{vd} << 52)
         | (std::uint64_t{rs1} << 47)
         | IMMEDIATE_MODE_BIT
         | (static_cast<std::uint64_t>(offset) & IMMEDIATE_MASK);
}

void runVld(
    control::ControlUnit& cu,
    unsigned vd,
    unsigned rs1,
    std::int64_t offset,
    std::uint64_t pc
) {
    cu.loadInstruction(encodeVld(vd, rs1, offset), pc);
    cu.executeInstruction();
}

} // namespace

int main() {
    control::ControlUnit cu;
    cu.loadProgram(control::buildDefaultMicrocode());

    // Test 1: Load three components into vector 0.
    // VLD V0, [R1 + 8]
    cu.writeRegister(1, 100);
    cu.writeMemoryWord(108, 0x11111111);
    cu.writeMemoryWord(112, 0x22222222);
    cu.writeMemoryWord(116, 0x33333333);

    runVld(cu, 0, 1, 8, 0);

    assert(static_cast<std::uint32_t>(cu.readRegister(20)) == 0x11111111);
    assert(static_cast<std::uint32_t>(cu.readRegister(21)) == 0x22222222);
    assert(static_cast<std::uint32_t>(cu.readRegister(22)) == 0x33333333);

    std::cout << "Test 1 passed: vector loaded with positive offset\n";

    // Test 2: Negative offset and a different destination vector.
    // VLD V1, [R2 - 4]
    cu.writeRegister(2, 204);
    cu.writeMemoryWord(200, 0xAAAAAAAA);
    cu.writeMemoryWord(204, 0xBBBBBBBB);
    cu.writeMemoryWord(208, 0xCCCCCCCC);

    runVld(cu, 1, 2, -4, 4);

    assert(static_cast<std::uint32_t>(cu.readRegister(23)) == 0xAAAAAAAA);
    assert(static_cast<std::uint32_t>(cu.readRegister(24)) == 0xBBBBBBBB);
    assert(static_cast<std::uint32_t>(cu.readRegister(25)) == 0xCCCCCCCC);

    std::cout << "Test 2 passed: vector loaded with negative offset\n";

    // Test 3: Loading a vector must not modify the scalar base register.
    assert(cu.readRegister(1) == 100);
    assert(cu.readRegister(2) == 204);

    std::cout << "Test 3 passed: scalar base registers unchanged\n";
    std::cout << "\nAll VLD tests passed.\n";

    return 0;
}
