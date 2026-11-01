
#include <cstdint>
#include <cstring>
#include <iostream>
#include <cmath>
#include <exception>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

static std::int32_t floatToBits(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));

    std::int32_t result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

static float bitsToFloat(std::int32_t value) {
    float result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

int main() {
    try {
        control::ControlUnit cu;
        control::MicrocodeProgram program =
            control::buildDefaultMicrocode();

        cu.loadProgram(program);

        cu.writeRegister(1, floatToBits(-7.5f));

        // FABS: R3 = abs(R1).
        const std::uint64_t instruction =
            (static_cast<std::uint64_t>(control::isa::FABS) << 57)
            | (static_cast<std::uint64_t>(3) << 52)
            | (static_cast<std::uint64_t>(1) << 47);

        cu.loadInstruction(instruction, 0);
        cu.executeInstruction();

        const float result = bitsToFloat(cu.readRegister(3));

        std::cout << "FABS result: " << result << '\n';

        if (std::fabs(result - 7.5f) > 0.00001f) {
            std::cerr << "FAIL: FABS expected 7.5\n";
            return 1;
        }

        std::cout << "FABS test passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
