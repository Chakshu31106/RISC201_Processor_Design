
#include <cstdint>
#include <iostream>
#include <exception>

#include "control_unit.h"
#include "microcode_setup.h"
#include "isa_opcodes.h"

int main() {
    try {
        control::ControlUnit cu;

        auto program = control::buildDefaultMicrocode();
        cu.loadProgram(program);

        // Test 1: Register mode: 48 ROTR 2 = 12.
        cu.writeRegister(1, 48);
        cu.writeRegister(2, 2);

        const std::uint64_t registerInstruction =
            (static_cast<std::uint64_t>(control::isa::ROTR) << 57)
            | (static_cast<std::uint64_t>(3) << 52)
            | (static_cast<std::uint64_t>(1) << 47)
            | (static_cast<std::uint64_t>(2) << 42);

        cu.loadInstruction(registerInstruction, 0);
        cu.executeInstruction();

        const std::int32_t registerResult = cu.readRegister(3);

        std::cout << "Register mode: R3 = "
                  << registerResult << '\n';

        if (registerResult != 12) {
            std::cerr << "FAIL: Register-mode ROTR\n";
            return 1;
        }

        // Test 2: Immediate mode: 40 ROTR 3 = 5.
        cu.writeRegister(1, 40);

        const std::uint64_t immediateInstruction =
            (static_cast<std::uint64_t>(control::isa::ROTR) << 57)
            | (static_cast<std::uint64_t>(4) << 52)
            | (static_cast<std::uint64_t>(1) << 47)
            | (static_cast<std::uint64_t>(1) << 41)
            | static_cast<std::uint64_t>(3);

        cu.loadInstruction(immediateInstruction, 0);
        cu.executeInstruction();

        const std::int32_t immediateResult = cu.readRegister(4);

        std::cout << "Immediate mode: R4 = "
                  << immediateResult << '\n';

        if (immediateResult != 5) {
            std::cerr << "FAIL: Immediate-mode ROTR\n";
            return 1;
        }

        // Test 3: Verify wraparound.
        // 0x00000001 ROTR 1 = 0x80000000.
        cu.writeRegister(1, 1);
        cu.writeRegister(2, 1);

        const std::uint64_t wrapInstruction =
            (static_cast<std::uint64_t>(control::isa::ROTR) << 57)
            | (static_cast<std::uint64_t>(5) << 52)
            | (static_cast<std::uint64_t>(1) << 47)
            | (static_cast<std::uint64_t>(2) << 42);

        cu.loadInstruction(wrapInstruction, 0);
        cu.executeInstruction();

        const std::int32_t wrapResult = cu.readRegister(5);

        std::cout << "Wraparound result: R5 = "
                  << wrapResult << '\n';

        if (wrapResult != INT32_MIN) {
            std::cerr << "FAIL: ROTR wraparound\n";
            return 1;
        }

        std::cout << "All ROTR tests passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
