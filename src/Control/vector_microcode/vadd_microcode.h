
#ifndef VADD_MICROCODE_H
#define VADD_MICROCODE_H

#include "vector_helpers.h"

namespace control {

inline void buildVaddRoutine(MicrocodeBuilder& b) {
    emitVectorBinaryRoutine(
        b, "VADD", isa::VADD, isa::FADD
    );
}

} // namespace control

#endif
