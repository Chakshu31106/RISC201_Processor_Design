
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

struct Vec3 {
    float x, y, z;
};

static Vec3 runCross(int destination, int va, int vb)
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

    // VCRS Vdestination, Va, Vb
    uint64_t instruction =
        (uint64_t(control::isa::VCRS) << 57)
        | (uint64_t(destination) << 52)
        | (uint64_t(va) << 47)
        | (uint64_t(vb) << 42);

    cu.loadInstruction(instruction, 0);
    cu.executeInstruction();

    const int base = 20 + destination * 3;

    return {
        bitsToFloat(cu.readRegister(base)),
        bitsToFloat(cu.readRegister(base + 1)),
        bitsToFloat(cu.readRegister(base + 2))
    };
}

static void checkCross(
    int destination,
    int va,
    int vb,
    Vec3 expected
)
{
    Vec3 actual = runCross(destination, va, vb);

    std::cout << "VCRS V" << destination
              << ", V" << va << ", V" << vb
              << " = (" << actual.x << ", "
              << actual.y << ", " << actual.z << ")\n";

    assert(std::fabs(actual.x - expected.x) < 1e-5f);
    assert(std::fabs(actual.y - expected.y) < 1e-5f);
    assert(std::fabs(actual.z - expected.z) < 1e-5f);
}

int main()
{
    // All nine ordered source-vector pairs.
    checkCross(0, 0, 0, {0, 0, 0});
    checkCross(0, 0, 1, {-3, 6, -3});
    checkCross(0, 0, 2, {2.5f, -5, 2.5f});

    checkCross(0, 1, 0, {3, -6, 3});
    checkCross(1, 1, 1, {0, 0, 0});
    checkCross(0, 1, 2, {7, -14, 7});

    checkCross(0, 2, 0, {-2.5f, 5, -2.5f});
    checkCross(0, 2, 1, {-7, 14, -7});
    checkCross(2, 2, 2, {0, 0, 0});

    // Different destination: V2 = V0 x V1.
    checkCross(2, 0, 1, {-3, 6, -3});

    // Destination overlaps the first source.
    checkCross(0, 0, 1, {-3, 6, -3});

    // Destination overlaps the second source.
    checkCross(1, 0, 1, {-3, 6, -3});

    std::cout << "All VCRS tests passed!\n";
    return 0;
}
