
#include <cassert>
#include <iostream>

#include "control_unit.h"

int main() {
    control::ControlUnit cu;

    cu.writeRegister(1, 15);
    cu.writeRegister(2, 25);

    assert(cu.readRegister(1) == 15);
    assert(cu.readRegister(2) == 25);

    cu.writeRegister(3, cu.readRegister(1) + cu.readRegister(2));

    assert(cu.readRegister(3) == 40);

    std::cout << "Control Unit register integration test passed.\n";
    return 0;
}
