
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

    // V0 = (1, 2, 3)
    cu.writeRegister(20, floatToBits(10.0f));
    cu.writeRegister(21, floatToBits(20.0f));
    cu.writeRegister(22, floatToBits(30.0f));

    // V1 = (10, 20, 30)
    cu.writeRegister(23, floatToBits(1.0f));
    cu.writeRegister(24, floatToBits(2.0f));
    cu.writeRegister(25, floatToBits(3.0f));

    // Execute the fixed VSUB operation: V0 = V1 - V2.
    // VSUB V1, V0, V1: V1 = V0 - V1
    uint64_t instruction =
        (uint64_t(control::isa::VSUB) << 57)
        | (uint64_t(2) << 52)  // rd = V2
        | (uint64_t(1) << 47)  // rs1 = V1
        | (uint64_t(0) << 42); // rs2 = V0

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

float x = bitsToFloat(cu.readRegister(26));
float y = bitsToFloat(cu.readRegister(27));
float z = bitsToFloat(cu.readRegister(28));

assert(std::fabs(x - (-9.0f)) < 1e-6f);
assert(std::fabs(y - (-18.0f)) < 1e-6f);
assert(std::fabs(z - (-27.0f)) < 1e-6f);

std::cout << "V2 = (" << x << ", " << y << ", " << z << ")\n";
std::cout << "VSUB reversed-order test passed!\n";
    return 0;
}
