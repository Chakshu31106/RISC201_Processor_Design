
#ifndef NOP_MICROCODE_H
#define NOP_MICROCODE_H

#include "microcode_builder.h"
#include "isa_opcodes.h"

namespace control {

inline void buildNopRoutine(MicrocodeBuilder& b) {
    b.beginRoutine("NOP", isa::NOP);

    // NOP performs no operation.
    // Return to the shared preamble.
    b.mb("MICROCODE_END");

    b.finishRoutine();
}

} // namespace control

#endif
