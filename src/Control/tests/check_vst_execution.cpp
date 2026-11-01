
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

// VST vs1, [rd + offset]
std::uint64_t encodeVst(
    unsigned vectorId,
    unsigned baseRegister,
    std::int64_t offset
) {
    assert(vectorId < 3);
    assert(baseRegister < 32);

    return (std::uint64_t{control::isa::VST} << 57)
         | (std::uint64_t{baseRegister} << 52)
         | (std::uint64_t{vectorId} << 47)
         | IMMEDIATE_MODE_BIT
         | (static_cast<std::uint64_t>(offset) & IMMEDIATE_MASK);
}

void runVst(
    control::ControlUnit& cu,
    unsigned vectorId,
    unsigned baseRegister,
    std::int64_t offset,
    std::uint64_t pc
) {
    cu.loadInstruction(
        encodeVst(vectorId, baseRegister, offset),
        pc
    );
    cu.executeInstruction();
}

} // namespace

int main() {
    control::ControlUnit cu;
    cu.loadProgram(control::buildDefaultMicrocode());

    // Test 1: Store V0 at base + positive offset.
    // VST V0, [R1 + 8]
    cu.writeRegister(1, 100);
    cu.writeRegister(20, 0x11111111);
    cu.writeRegister(21, 0x22222222);
    cu.writeRegister(22, 0x33333333);

    runVst(cu, 0, 1, 8, 0);

    assert(static_cast<std::uint32_t>(cu.readMemoryWord(108))
           == 0x11111111U);
    assert(static_cast<std::uint32_t>(cu.readMemoryWord(112))
           == 0x22222222U);
    assert(static_cast<std::uint32_t>(cu.readMemoryWord(116))
           == 0x33333333U);

    std::cout << "Test 1 passed: positive offset\n";

    // Test 2: Store V1 at base + negative offset.
    // VST V1, [R2 - 4]
    cu.writeRegister(2, 204);
    cu.writeRegister(23, 0xAAAAAAAA);
    cu.writeRegister(24, 0xBBBBBBBB);
    cu.writeRegister(25, 0xCCCCCCCC);

    runVst(cu, 1, 2, -4, 4);

    assert(static_cast<std::uint32_t>(cu.readMemoryWord(200))
           == 0xAAAAAAAAU);
    assert(static_cast<std::uint32_t>(cu.readMemoryWord(204))
           == 0xBBBBBBBBU);
    assert(static_cast<std::uint32_t>(cu.readMemoryWord(208))
           == 0xCCCCCCCCU);

    std::cout << "Test 2 passed: negative offset\n";

    // Test 3: Base registers remain unchanged.
    assert(cu.readRegister(1) == 100);
    assert(cu.readRegister(2) == 204);

    std::cout << "Test 3 passed: base registers unchanged\n";
    std::cout << "\nAll VST tests passed.\n";

    return 0;
}
