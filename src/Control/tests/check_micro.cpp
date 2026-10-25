
#include <cassert>

#include "microinstruction.h"
#include "internal_registers.h"

int main() {
    using namespace control;

    assert(INTERNAL_REGISTER_COUNT == 31);

    assert(registerIndex(InternalRegister::PC) == 0);
    assert(registerIndex(InternalRegister::IR) == 1);
    assert(registerIndex(InternalRegister::ALU_RESULT) == 17);
    assert(registerIndex(InternalRegister::TMP4) == 30);

    Microinstruction m{};
    m.type = MicroType::MMOV;
    m.src = static_cast<int>(registerIndex(InternalRegister::PC));
    m.dest = static_cast<int>(registerIndex(InternalRegister::IR));

    assert(m.src == 0);
    assert(m.dest == 1);

    return 0;
}
