
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

static float runMagnitude(
    int destination,
    int source,
    float x,
    float y,
    float z
)
{
    control::ControlUnit cu;

    auto program = control::buildDefaultMicrocode();
    cu.loadProgram(program);

    // Initialize the selected source vector.
    const int base = 20 + source * 3;

    cu.writeRegister(base,     floatToBits(x));
    cu.writeRegister(base + 1, floatToBits(y));
    cu.writeRegister(base + 2, floatToBits(z));

    // VMAG Rd, Vs
    uint64_t instruction =
        (uint64_t(control::isa::VMAG) << 57)
        | (uint64_t(destination) << 52)
        | (uint64_t(source) << 47);

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    return bitsToFloat(cu.readRegister(destination));
}

static void checkMagnitude(
    int destination,
    int source,
    float x,
    float y,
    float z,
    float expected
)
{
    float actual = runMagnitude(
        destination, source, x, y, z
    );

    std::cout << "VMAG R" << destination
              << ", V" << source
              << " = " << actual
              << " (expected " << expected << ")"
              << std::endl;

    assert(std::isfinite(actual));
    assert(std::fabs(actual - expected) < 1e-4f);
}

int main()
{
    // Magnitude of (1, 2, 2) = sqrt(1 + 4 + 4) = 3.
    checkMagnitude(10, 0, 1, 2, 2, 3.0f);

    // Magnitude of (3, 4, 0) = sqrt(9 + 16) = 5.
    checkMagnitude(10, 1, 3, 4, 0, 5.0f);

    // Magnitude of (-1, 1, 1) = sqrt(3).
    checkMagnitude(10, 2, -1, 1, 1, std::sqrt(3.0f));

    // Actual zero-vector test.
    checkMagnitude(10, 0, 0, 0, 0, 0.0f);

    // Different scalar destination register.
    checkMagnitude(15, 1, 3, 4, 0, 5.0f);

    std::cout << "All VMAG tests passed!" << std::endl;
    return 0;
}
