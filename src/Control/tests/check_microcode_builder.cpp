
#include <cassert>
#include <cstdint>

#include "microcode_builder.h"

int main() {
    using namespace control;

    MicrocodeBuilder builder;
    MicrocodeProgram program;

    // Routine 1: shared preamble, with no ISA opcode.
    builder.beginRoutine("preamble", -1);

    Microinstruction load{};
    load.type = MicroType::MLOAD_IR;
    builder.append(load);

    builder.finishRoutine();

    // Routine 2: test a forward jump and a cross-routine jump.
    builder.beginRoutine("routine_add", 0x00);

    
    builder.mb("later");

    builder.label("later");

    Microinstruction move{};
    move.type = MicroType::MMOV;
    builder.append(move);

    builder.mb("preamble");


    builder.finishRoutine();

    // Routine 3: jump into another routine.
    builder.beginRoutine("routine_sub", 0x01);

    builder.mb("routine_add");

    builder.finishRoutine();

    // Resolve all labels and construct Control Memory.
    builder.finalize(program);

    // Check routine entry addresses.
    assert(program.getEntryAddress(0x00) == 1);
    assert(program.getEntryAddress(0x01) == 4);

    // Check total Control Memory size.
    assert(program.size() == 5);

    // Address 0: preamble.
    assert(program.controlMemory[0].type == MicroType::MLOAD_IR);

    // Address 1: forward jump to "later", at address 2.
    assert(program.controlMemory[1].type == MicroType::MB);
    assert(program.controlMemory[1].seqAddr == 2);

    // Address 2: MMOV.
    assert(program.controlMemory[2].type == MicroType::MMOV);

    // Address 3: jump from routine_add to preamble.
    assert(program.controlMemory[3].type == MicroType::MB);
    assert(program.controlMemory[3].seqAddr == 0);

    // Address 4: jump from routine_sub to routine_add.
    assert(program.controlMemory[4].type == MicroType::MB);
    assert(program.controlMemory[4].seqAddr == 1);

    return 0;
}
