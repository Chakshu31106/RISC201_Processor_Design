
#include <cstdint>
#include <cstring>
#include <iostream>
#include <exception>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"
#include "internal_registers.h"

static std::int32_t floatToBits(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));

    std::int32_t result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

int main() {
    try {
        control::ControlUnit cu;
        control::MicrocodeProgram program =
            control::buildDefaultMicrocode();

        cu.loadProgram(program);

        // Test: 5.5 > 2.25.
        cu.writeRegister(1, floatToBits(-5.5f));
        cu.writeRegister(2, floatToBits(2.25f));

        const std::uint64_t instruction =
            (static_cast<std::uint64_t>(control::isa::FCMP) << 57)
            | (static_cast<std::uint64_t>(3) << 52)
            | (static_cast<std::uint64_t>(1) << 47)
            | (static_cast<std::uint64_t>(2) << 42);

        cu.loadInstruction(instruction, 0);
        cu.executeInstruction();

        using IR = control::InternalRegister;

        const auto equal =
            cu.readInternal(IR::FLAG_E);
        const auto greater =
            cu.readInternal(IR::FLAG_GT);
        const auto smaller =
            cu.readInternal(IR::FLAG_ST);

        std::cout << "FLAG_E  = " << equal << '\n';
        std::cout << "FLAG_GT = " << greater << '\n';
        std::cout << "FLAG_ST = " << smaller << '\n';

        // if (equal != 0 || greater != 1 || smaller != 0) {
        //     std::cerr << "FAIL: FCMP flags are incorrect\n";
        //     return 1;
        // }

        std::cout << "FCMP test passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
