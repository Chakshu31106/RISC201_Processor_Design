
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

namespace {

std::uint32_t floatBits(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

float bitsFloat(std::int32_t value) {
    const std::uint32_t bits = static_cast<std::uint32_t>(value);
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

void setFloatRegister(
    control::ControlUnit& cu,
    unsigned reg,
    float value
) {
    cu.writeRegister(
        reg,
        static_cast<std::int32_t>(floatBits(value))
    );
}

std::uint64_t encodeVang(
    unsigned rd,
    unsigned vs1,
    unsigned vs2
) {
    return (std::uint64_t{control::isa::VANG} << 57)
         | (std::uint64_t{rd} << 52)
         | (std::uint64_t{vs1} << 47)
         | (std::uint64_t{vs2} << 42);
}

float runVang(
    control::ControlUnit& cu,
    unsigned rd,
    unsigned vs1,
    unsigned vs2,
    std::uint64_t pc
) {
    cu.loadInstruction(encodeVang(rd, vs1, vs2), pc);
    cu.executeInstruction();
    return bitsFloat(cu.readRegister(rd));
}

bool near(float actual, float expected, float tolerance = 0.02f) {
    return std::fabs(actual - expected) <= tolerance;
}

} // namespace

int main() {
    control::ControlUnit cu;
    cu.loadProgram(control::buildDefaultMicrocode());

    // V0 = (1, 0, 0)
    setFloatRegister(cu, 20, 1.0f);
    setFloatRegister(cu, 21, 0.0f);
    setFloatRegister(cu, 22, 0.0f);

    // V1 = (0, 1, 0)
    setFloatRegister(cu, 23, 0.0f);
    setFloatRegister(cu, 24, 1.0f);
    setFloatRegister(cu, 25, 0.0f);

    // V2 = (-1, 0, 0)
    setFloatRegister(cu, 26, -1.0f);
    setFloatRegister(cu, 27, 0.0f);
    setFloatRegister(cu, 28, 0.0f);

    // Test 1: perpendicular vectors -> pi/2 radians.
    const float rightAngle = runVang(cu, 10, 0, 1, 0);
    assert(near(rightAngle, 1.5707963f));
    std::cout << "Test 1 passed: perpendicular vectors\n";

    // Test 2: same vector -> 0 radians.
    const float zeroAngle = runVang(cu, 11, 0, 0, 8);
    assert(near(zeroAngle, 0.0f));
    std::cout << "Test 2 passed: identical vectors\n";

    // Test 3: opposite vectors -> pi radians.
    const float straightAngle = runVang(cu, 12, 0, 2, 16);
    assert(near(straightAngle, 3.1415926f));
    std::cout << "Test 3 passed: opposite vectors\n";

    std::cout << "\nAll VANG tests passed.\n";
    return 0;
}
