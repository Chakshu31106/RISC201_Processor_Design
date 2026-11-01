
#include <cassert>
#include <iostream>

#include "control_unit.h"
#include "isa_opcodes.h"

int main() {
    control::ControlUnit cu;
    control::MicrocodeProgram program;

    // Microinstruction: B = A + B
    control::Microinstruction add{};
    add.type = control::MicroType::MMOV;
    add.src = static_cast<int>(control::InternalRegister::A);
    add.dest = static_cast<int>(control::InternalRegister::B);
    add.unit = control::UnitId::ALU;
    add.opcode = control::isa::ADD;

    // Register the routine under the ADD opcode.
    program.addRoutine(
        static_cast<std::uint8_t>(control::isa::ADD),
        std::vector<control::Microinstruction>{add}
    );

    cu.loadProgram(program);

    // Set operands.
    cu.writeInternal(control::InternalRegister::A, 10);
    cu.writeInternal(control::InternalRegister::B, 20);

    // Load an ADD instruction to activate the microcode.
    // The instruction's opcode occupies bits 63–57.
    const std::uint64_t encodedAdd =
        static_cast<std::uint64_t>(control::isa::ADD) << 57;

    cu.loadInstruction(encodedAdd, 0);

    // Execute the microinstruction.
    assert(cu.stepMicroinstruction());

    const auto result =
        cu.readInternal(control::InternalRegister::ALU_RESULT);

    std::cout << "Result: " << result << '\n';
    assert(result == 30);

    std::cout << "ALU dispatch test passed.\n";
    return 0;
}
