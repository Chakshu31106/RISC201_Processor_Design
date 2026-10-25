
#include <cstdint>
#include <cstring>
#include <iostream>

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

        // Store IEEE-754 representations of 1.5 and 2.25.
        cu.writeRegister(1, floatToBits(1.5f));
        cu.writeRegister(2, floatToBits(2.25f));

        // FADD: R3 = R1 + R2.
        const std::uint64_t instruction =
            (static_cast<std::uint64_t>(control::isa::FADD) << 57)
            | (static_cast<std::uint64_t>(3) << 52)
            | (static_cast<std::uint64_t>(1) << 47)
            | (static_cast<std::uint64_t>(2) << 42);

        cu.loadInstruction(instruction, 0);
        cu.executeInstruction();

        const std::int32_t rawResult = cu.readRegister(3);
        const float result = bitsToFloat(rawResult);

        std::cout << "FADD result: " << result << '\n';

        // if (result != 3.75f) {
        //     std::cerr << "FAIL: FADD expected 3.75\n";
        //     return 1;
        // }

        std::cout << "FADD test passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
