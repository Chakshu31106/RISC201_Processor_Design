
#ifndef ISA_OPCODES_H
#define ISA_OPCODES_H

#include <cstdint>

namespace control {
namespace isa {

// Arithmetic
constexpr std::uint8_t ADD  = 0x00;
constexpr std::uint8_t SUB  = 0x01;
constexpr std::uint8_t MUL  = 0x02;
constexpr std::uint8_t DIV  = 0x03;
constexpr std::uint8_t MOD  = 0x04;
constexpr std::uint8_t CMP  = 0x05;
constexpr std::uint8_t INCR = 0x06;
constexpr std::uint8_t DECR = 0x07;
constexpr std::uint8_t POW  = 0x08;
constexpr std::uint8_t ABS  = 0x09;

// Logical
constexpr std::uint8_t AND  = 0x10;
constexpr std::uint8_t OR   = 0x11;
constexpr std::uint8_t NOT  = 0x12;
constexpr std::uint8_t NOR  = 0x13;
constexpr std::uint8_t NAND = 0x14;
constexpr std::uint8_t XOR  = 0x15;
constexpr std::uint8_t LSL  = 0x16;
constexpr std::uint8_t LSR  = 0x17;
constexpr std::uint8_t ASR  = 0x18;
constexpr std::uint8_t ROTL = 0x19;
constexpr std::uint8_t ROTR = 0x1A;

// Bit manipulation
constexpr std::uint8_t PCNT = 0x20;
constexpr std::uint8_t TOG  = 0x21;
constexpr std::uint8_t SBIT = 0x22;
constexpr std::uint8_t CBIT = 0x23;

// Branch
constexpr std::uint8_t CALL = 0x30;
constexpr std::uint8_t RET  = 0x31;
constexpr std::uint8_t B    = 0x32;
constexpr std::uint8_t BEQ  = 0x33;
constexpr std::uint8_t BGT  = 0x34;
constexpr std::uint8_t BST  = 0x35;

// Memory
constexpr std::uint8_t LD   = 0x40;
constexpr std::uint8_t ST   = 0x41;
constexpr std::uint8_t MOV  = 0x42;

// Stack
constexpr std::uint8_t PUSH = 0x50;
constexpr std::uint8_t POP  = 0x51;
constexpr std::uint8_t MPOP = 0x52;
constexpr std::uint8_t PEEK = 0x53;
constexpr std::uint8_t CLR  = 0x54;

// Floating point
constexpr std::uint8_t FADD = 0x60;
constexpr std::uint8_t FSUB = 0x61;
constexpr std::uint8_t FMUL = 0x62;
constexpr std::uint8_t FDIV = 0x63;
constexpr std::uint8_t FCMP = 0x64;
constexpr std::uint8_t SQRT = 0x65;
constexpr std::uint8_t FABS = 0x66;
constexpr std::uint8_t SIN  = 0x67;
constexpr std::uint8_t COS  = 0x68;
constexpr std::uint8_t ACOS = 0x69;

// Vector
constexpr std::uint8_t VADD = 0x70;
constexpr std::uint8_t VSUB = 0x71;
constexpr std::uint8_t VDOT = 0x72;
constexpr std::uint8_t VCRS = 0x73;
constexpr std::uint8_t VMAG = 0x74;
constexpr std::uint8_t VANG = 0x75;
constexpr std::uint8_t VLD  = 0x76;
constexpr std::uint8_t VST  = 0x77;

// Miscellaneous
constexpr std::uint8_t NOP  = 0x7C;

} 
} 

#endif
