
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "control_unit.h"
#include "instruction_decoder.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

namespace {

constexpr std::uint64_t IMMEDIATE_MODE_BIT = std::uint64_t{1} << 41;
constexpr std::uint64_t IMMEDIATE_MASK =
    (std::uint64_t{1} << 41) - 1;

// Encode ST rs1, [rd + offset].
//
// Instruction layout:
// opcode [63:57], rd [56:52], rs1 [51:47],
// I [41], immediate [40:0].
std::uint64_t encodeSt(
    unsigned rd,
    unsigned rs1,
    std::int64_t offset
) {
    assert(rd < 32);
    assert(rs1 < 32);

    return (std::uint64_t{control::isa::ST} << 57)
         | (std::uint64_t{rd} << 52)
         | (std::uint64_t{rs1} << 47)
         | IMMEDIATE_MODE_BIT
         | (static_cast<std::uint64_t>(offset) & IMMEDIATE_MASK);
}

void runSt(
    control::ControlUnit& cu,
    unsigned rd,
    unsigned rs1,
    std::int64_t offset,
    std::uint64_t pc
) {
    const std::uint64_t instruction = encodeSt(rd, rs1, offset);

    const auto decoded =
        control::InstructionDecoder::decode(instruction);

    assert(decoded.opcode == control::isa::ST);
    assert(decoded.rd == rd);
    assert(decoded.rs1 == rs1);
    assert(decoded.immediateMode);
    assert(decoded.immediate == offset);

    cu.loadInstruction(instruction, pc);
    cu.executeInstruction();
}

} // namespace

int main() {
    control::ControlUnit cu;
    cu.loadProgram(control::buildDefaultMicrocode());

    // Test 1: Positive offset.
    // ST R2, [R1 + 8]
    cu.writeRegister(1, 100);   // Base address
    cu.writeRegister(2, 12345); // Value to store

    runSt(cu, 1, 2, 8, 0);

    assert(cu.readMemoryWord(108) == 12345);
    assert(cu.readRegister(1) == 100);
    assert(cu.readRegister(2) == 12345);

    std::cout << "Test 1 passed: positive offset\n";

    // Test 2: Negative offset.
    // ST R4, [R3 - 8]
    cu.writeRegister(3, 200);
    cu.writeRegister(4, 54321);

    runSt(cu, 3, 4, -8, 8);

    assert(cu.readMemoryWord(192) == 54321);
    assert(cu.readRegister(3) == 200);
    assert(cu.readRegister(4) == 54321);

    std::cout << "Test 2 passed: negative offset\n";

    // Test 3: Zero offset.
    // ST R6, [R5 + 0]
    cu.writeRegister(5, 300);
    cu.writeRegister(6, 98765);

    runSt(cu, 5, 6, 0, 16);

    assert(cu.readMemoryWord(300) == 98765);

    std::cout << "Test 3 passed: zero offset\n";

    // Test 4: Highest valid word address in 64 KiB memory.
    cu.writeRegister(7, 65532);
    cu.writeRegister(8, 0x12345678);

    runSt(cu, 7, 8, 0, 24);

    assert(cu.readMemoryWord(65532) == 0x12345678U);

    std::cout << "Test 4 passed: last valid word address\n";

    // Test 5: Out-of-range address must be rejected.
    cu.writeRegister(9, 65533);
    cu.writeRegister(10, 777);

    bool threw = false;

    try {
        runSt(cu, 9, 10, 0, 32);
    } catch (const std::out_of_range&) {
        threw = true;
    }

    assert(threw);

    std::cout << "Test 5 passed: out-of-range address rejected\n";

    std::cout << "\nAll ST tests passed.\n";

    return 0;
}
