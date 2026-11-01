
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

static float runDot(int rd, int v1, int v2)
{
    control::ControlUnit cu;
    auto program = control::buildDefaultMicrocode();
    cu.loadProgram(program);

    // V0 = (1, 2, 3)
    cu.writeRegister(20, floatToBits(1.0f));
    cu.writeRegister(21, floatToBits(2.0f));
    cu.writeRegister(22, floatToBits(3.0f));

    // V1 = (4, 5, 6)
    cu.writeRegister(23, floatToBits(4.0f));
    cu.writeRegister(24, floatToBits(5.0f));
    cu.writeRegister(25, floatToBits(6.0f));

    // V2 = (-1, 0.5, 2)
    cu.writeRegister(26, floatToBits(-1.0f));
    cu.writeRegister(27, floatToBits(0.5f));
    cu.writeRegister(28, floatToBits(2.0f));

    uint64_t instruction =
        (uint64_t(control::isa::VDOT) << 57)
        | (uint64_t(rd) << 52)
        | (uint64_t(v1) << 47)
        | (uint64_t(v2) << 42);

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    return bitsToFloat(
        static_cast<uint32_t>(cu.readRegister(rd))
    );
}

static void checkDot(
    int rd, int v1, int v2, float expected
)
{
    float actual = runDot(rd, v1, v2);

    std::cout << "VDOT R" << rd
              << ", V" << v1
              << ", V" << v2
              << " = " << actual << '\n';

    assert(std::fabs(actual - expected) < 1e-5f);
}

int main()
{
    // All ordered pairs among V0, V1, and V2.
    checkDot(5, 0, 0, 14.0f);
    checkDot(5, 0, 1, 32.0f);
    checkDot(5, 0, 2, 6.0f);

    checkDot(5, 1, 0, 32.0f);
    checkDot(5, 1, 1, 77.0f);
    checkDot(5, 1, 2, 10.5f);

    checkDot(5, 2, 0, 6.0f);
    checkDot(5, 2, 1, 10.5f);
    checkDot(5, 2, 2, 5.25f);

    // Different scalar destination register.
    checkDot(10, 1, 2, 10.5f);

    // Scalar destination overlaps V1's X component (R23).
    // The dot product must be calculated before R23 is overwritten.
    checkDot(23, 1, 2, 10.5f);

    std::cout << "All VDOT tests passed!\n";
    return 0;
}
