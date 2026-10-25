
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

static uint32_t floatToBits(float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static float bitsToFloat(uint32_t bits)
{
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

int main()
{
    control::ControlUnit cu;
    auto program = control::buildDefaultMicrocode();
    cu.loadProgram(program);

    // V1 = (1, 2, 3), stored in R23, R24, R25.
    cu.writeRegister(20, floatToBits(10.0f));
    cu.writeRegister(21, floatToBits(20.0f));
    cu.writeRegister(22, floatToBits(30.0f));

    // V2 = (4, 5, 6), stored in R26, R27, R28.
    cu.writeRegister(23, floatToBits(1.0f));
    cu.writeRegister(24, floatToBits(2.0f));
    cu.writeRegister(25, floatToBits(3.0f));

    // Execute the fixed VSUB operation: V0 = V1 + V2.
    // VADD V1, V0, V1: V1 = V0 + V1
    uint64_t instruction =
        (uint64_t(control::isa::VADD) << 57)
        | (uint64_t(1) << 52)  // rd = V1
        | (uint64_t(0) << 47)  // rs1 = V0
        | (uint64_t(1) << 42); // rs2 = V1

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    float x = bitsToFloat(cu.readRegister(23));
    float y = bitsToFloat(cu.readRegister(24));
    float z = bitsToFloat(cu.readRegister(25));

assert(std::fabs(x - 11.0f) < 1e-6f);
assert(std::fabs(y - 22.0f) < 1e-6f);
assert(std::fabs(z - 33.0f) < 1e-6f);

    std::cout << "V0 = (" << x << ", " << y << ", " << z << ")\n";
    std::cout << "VADD test passed!\n";

    return 0;
}
