#include <cassert>
#include <iostream>
#include <set>
#include <vector>

#include "microcode_setup.h"
#include "isa_opcodes.h"

int main() {
    control::MicrocodeProgram program =
        control::buildDefaultMicrocode();

    // Every opcode that should have a registered routine.
    const std::vector<std::uint8_t> expectedOpcodes = {
        control::isa::ADD,
        control::isa::SUB,
        control::isa::MUL,
        control::isa::DIV,
        control::isa::MOD,
        control::isa::CMP,
        control::isa::INCR,
        control::isa::DECR,
        control::isa::POW,
        control::isa::ABS,

        control::isa::AND,
        control::isa::OR,
        control::isa::NOT,
        control::isa::NOR,
        control::isa::NAND,
        control::isa::XOR,
        control::isa::LSL,
        control::isa::LSR,
        control::isa::ASR,
        control::isa::ROTL,
        control::isa::ROTR,
        control::isa::PCNT,
        control::isa::TOG,
        control::isa::SBIT,
        control::isa::CBIT,

        control::isa::BEQ,
        control::isa::BGT,
        control::isa::BST,
        control::isa::B,
        control::isa::CALL,
        control::isa::RET,

        control::isa::FADD,
        control::isa::FSUB,
        control::isa::FMUL,
        control::isa::FDIV,
        control::isa::FCMP,
        control::isa::SQRT,
        control::isa::FABS,
        control::isa::SIN,
        control::isa::COS,

        control::isa::VADD,
        control::isa::VSUB,
        control::isa::VDOT,
        control::isa::VCRS,
        control::isa::VMAG,

        control::isa::LD,
        control::isa::ST,
        control::isa::MOV,
        control::isa::NOP,
        control::isa::VLD,
        control::isa::VST,
        control::isa::VANG
    };

    std::set<int> uniqueEntries;

    for (const auto opcode : expectedOpcodes) {
        const int entry = program.getEntryAddress(opcode);

        assert(entry >= 0);
        assert(static_cast<std::size_t>(entry) < program.size());

        // Each registered opcode should start at a distinct address.
        assert(uniqueEntries.insert(entry).second);
    }

    assert(program.size() > 0);

    std::cout << "Registered routines checked: "
              << expectedOpcodes.size() << '\n';

    std::cout << "Total microinstructions: "
              << program.size() << '\n';

    std::cout << "Microcode setup checks passed!\n";

    return 0;
}