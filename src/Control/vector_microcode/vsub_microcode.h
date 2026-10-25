
#ifndef VSUB_MICROCODE_H
#define VSUB_MICROCODE_H

#include "vector_helpers.h"

namespace control {

inline void buildVsubRoutine(MicrocodeBuilder& b) {
    emitVectorBinaryRoutine(
        b, "VSUB", isa::VSUB, isa::FSUB
    );
}

} // namespace control

#endif
