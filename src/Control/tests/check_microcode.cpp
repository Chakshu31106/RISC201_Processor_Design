#include <cassert>
#include <cstdint>
#include <vector>

#include "microinstruction.h"
#include "microcode.h"

int main() {
    using namespace control;

    MicrocodeProgram program;

    // Initially, no routine is registered.
    assert(program.getEntryAddress(0x00) == -1);
    assert(program.size() == 0);

    // Create a tiny test routine.
    // These are test microinstructions, not the actual ADD routine.
    Microinstruction m1{};
    m1.type = MicroType::MMOV;

    Microinstruction m2{};
    m2.type = MicroType::MADD;

    std::vector<Microinstruction> testRoutine = {
        m1,
        m2
    };

    // Register the test routine under opcode 0x00.
    int start = program.addRoutine(0x00, testRoutine);

    // The first routine starts at address zero.
    assert(start == 0);
    assert(program.getEntryAddress(0x00) == 0);

    // Two microinstructions were added.
    assert(program.size() == 2);

    // Check that the instructions are stored in order.
    assert(program.controlMemory[0].type == MicroType::MMOV);
    assert(program.controlMemory[1].type == MicroType::MADD);

    // Register a second test routine.
    Microinstruction m3{};
    m3.type = MicroType::MLOAD_IR;

    std::vector<Microinstruction> secondRoutine = {m3};

    int secondStart = program.addRoutine(0x01, secondRoutine);

    // It starts immediately after the first routine.
    assert(secondStart == 2);
    assert(program.getEntryAddress(0x01) == 2);
    assert(program.size() == 3);

    return 0;
}