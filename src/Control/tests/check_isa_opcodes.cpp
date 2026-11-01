
#include <cassert>
#include "isa_opcodes.h"

int main() {
    using namespace control::isa;

    assert(ADD  == 0x00);
    assert(CMP  == 0x05);
    assert(AND  == 0x10);
    assert(PCNT == 0x20);
    assert(CALL == 0x30);
    assert(LD   == 0x40);
    assert(FADD == 0x60);
    assert(VADD == 0x70);
    assert(NOP  == 0x7C);

    return 0;
}
