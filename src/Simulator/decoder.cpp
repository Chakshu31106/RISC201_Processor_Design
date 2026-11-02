#include "Simulator/decoder.h"

#include "Assembler/risc201_common.h"

namespace risc201 {

// Bit positions mirror the disassembler, which is the exact inverse of the
// encoder, so the CPU and assembler cannot disagree on the layout.

DecodedInstr decode(uint64_t word) {
    DecodedInstr d;
    d.raw    = word;
    d.opcode = static_cast<uint8_t>(extract_field(word, 63, 57));

    auto it = kOpcodeByValue.find(d.opcode);
    if (it == kOpcodeByValue.end()) return d;   // valid stays false

    d.valid    = true;
    d.mnemonic = it->second.mnemonic;
    d.format   = it->second.format;

    // Common field slots (only copied into the struct where a format uses them).
    const uint8_t f_rd  = static_cast<uint8_t>(extract_field(word, 56, 52));
    const uint8_t f_rs1 = static_cast<uint8_t>(extract_field(word, 51, 47));
    const uint8_t f_rs2 = static_cast<uint8_t>(extract_field(word, 46, 42));
    const bool    f_I   = extract_field(word, 41, 41) != 0;
    const int64_t imm42 = from_signed_field(extract_field(word, 41, 0), 42);

    switch (d.format) {
        case Format::R:
        case Format::R_IMM:
            d.rd = f_rd;
            d.rs1 = f_rs1;
            d.use_imm = f_I;
            if (f_I) d.imm = from_signed_field(extract_field(word, 40, 0), 41);
            else     d.rs2 = f_rs2;
            break;

        case Format::CMP:
            d.rs1 = f_rs1;
            d.use_imm = f_I;
            if (f_I) d.imm = from_signed_field(extract_field(word, 40, 0), 41);
            else     d.rs2 = f_rs2;
            break;

        case Format::R1:
        case Format::FP_R1:
        case Format::VEC_SCALAR1:
            d.rd = f_rd;
            d.rs1 = f_rs1;
            break;

        case Format::R1S:
            d.rs1 = f_rs1;
            break;

        case Format::BIT:
            d.rd = f_rd;
            d.rs1 = f_rs1;
            d.bit_idx = f_rs2;          // [46:42]
            break;

        case Format::BRANCH:
        case Format::STACK_N_COUNT:
            d.imm = imm42;
            break;

        case Format::MEM_LOAD:
        case Format::MEM_STORE:
        case Format::VEC_LOAD:
        case Format::VEC_STORE:
            d.rd = f_rd;
            d.rs1 = f_rs1;
            d.imm = imm42;
            break;

        case Format::MOVE:
            d.rd = f_rd;
            d.use_imm = f_I;
            d.mode = static_cast<uint8_t>(extract_field(word, 40, 39));
            if (f_I) d.imm = from_signed_field(extract_field(word, 38, 0), 39);
            else     d.rs2 = f_rs2;
            break;

        case Format::STACK_1:
            d.rd = f_rd;
            break;

        case Format::FP_R:
        case Format::VEC_3REG:
        case Format::VEC_SCALAR2:
            d.rd = f_rd;
            d.rs1 = f_rs1;
            d.rs2 = f_rs2;
            break;

        case Format::FP_CMP:
            d.rs1 = f_rs1;
            d.rs2 = f_rs2;
            break;

        case Format::BRANCH_NOOP:
        case Format::STACK_N_NOOP:
        case Format::NO_OPERAND:
            break;
    }
    return d;
}

}  // namespace risc201
