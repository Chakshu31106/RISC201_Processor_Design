
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "control_unit.h"
#include "isa_opcodes.h"

using namespace control;

int main() { 
    MicrocodeProgram program;

    // Shared preamble begins at microaddress 0.
    std::vector<Microinstruction> preamble = {
        {MicroType::MLOAD_IR},
        {MicroType::MDECODE},
        {MicroType::MSWITCH}
    };

    program.appendUnmapped(preamble);

    // ADD routine begins immediately after the preamble.
    std::vector<Microinstruction> addRoutine = {
        {MicroType::MMOVI, -1,
         registerIndex(InternalRegister::TMP1), 123},
        {MicroType::MB, -1, -1, 0, 0}
    };

    int addEntry = program.addRoutine(isa::ADD, addRoutine);

    assert(addEntry == 3);
    assert(program.getEntryAddress(isa::ADD) == 3);

    ControlUnit cu;
    cu.loadProgram(program);

    // Construct an ADD instruction with opcode ADD.
    // The other fields are zero for this control-flow test.
    std::uint64_t instruction =
        static_cast<std::uint64_t>(isa::ADD) << 57;

    cu.loadInstruction(instruction, 100);

    assert(cu.getMicroPC() == 0);

    // Execute microinstructions until the instruction completes.
    cu.executeInstruction();
    assert(cu.readInternal(InternalRegister::TMP1) == 123);
    assert(cu.getMicroPC() == 0);

    std::cout << "Microcode flow test passed.\n";
    std::cout << "ADD entry address: " << addEntry << '\n';

    return 0;
}
