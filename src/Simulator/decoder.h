#pragma once

#include <cstdint>
#include <string>

#include "Assembler/risc201_common.h"

namespace risc201 {

// Result of decoding one 64-bit instruction word.
//
// Fields are filled in according to the instruction's Format; fields a format
// doesn't use are left at 0. Field meaning per format (matches the encoder):
//
//   R / R_IMM   rd, rs1, then rs2 (use_imm=false) or imm (use_imm=true)
//   CMP         rs1, then rs2 or imm
//   R1 / FP_R1  rd, rs1
//   R1S         rs1 only (incr/decr/abs: read and write back to rs1)
//   BIT         rd, rs1, bit_idx
//   BRANCH      imm = offset relative to the branch's OWN address
//   MEM_LOAD    rd = destination, rs1 = base, imm = offset
//   MEM_STORE   rd = BASE register, rs1 = VALUE register, imm = offset
//   MOVE        rd; use_imm=false -> rs2, mode;  use_imm=true -> imm, mode
//   STACK_1     rd
//   STACK_N_COUNT  imm = count
//   FP_R / VEC_3REG / VEC_SCALAR2   rd, rs1, rs2
//   FP_CMP      rs1, rs2
//   VEC_LOAD    rd = vector dest, rs1 = base, imm = offset
//   VEC_STORE   rd = BASE register, rs1 = VALUE vector, imm = offset
//   VEC_SCALAR1 rd, rs1
//   *_NOOP / NO_OPERAND   nothing
struct DecodedInstr {
    bool        valid    = false;   // false if the opcode is unknown
    uint64_t    raw      = 0;
    uint8_t     opcode   = 0;
    std::string mnemonic;
    Format      format   = Format::NO_OPERAND;

    uint8_t     rd       = 0;
    uint8_t     rs1      = 0;
    uint8_t     rs2      = 0;
    bool        use_imm  = false;   // the I-bit, for formats that have one
    int64_t     imm      = 0;       // sign-extended
    uint8_t     mode     = 0;       // mov only: 0 normal, 1 unsigned, 2 movh
    uint8_t     bit_idx  = 0;       // tog / sbit / cbit
};

// Decodes one 64-bit instruction word. Never throws: an unknown opcode
// returns an instruction with valid == false.
DecodedInstr decode(uint64_t word);

}  // namespace risc201
