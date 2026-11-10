// Shared RISC201 implementation: ISA table, parsing, encoding and decoding.
#pragma once
// RISC201 simplified single-file assembler/disassembler
// Flattened from the original multi-file project to make navigation easier.
// The instruction formats and encoding rules are preserved from that project.
// Build: g++ -std=c++17 -Wall risc201_simple.cpp -o risc201

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>



// ===== Declarations from token.h =====

// Output of the tokenizer, and input to Pass 1 / Pass 2.
struct Token {
    std::string label;                     // empty if no label on this line
    std::string mnemonic;                  // empty if label-only line
    std::vector<std::string> operands;     // raw operand strings, unparsed
    uint32_t address = 0;                  // filled in by Pass 1
    int line_number = 0;                   // 1-based source line — for error messages
};

// ===== Declarations from opcode_table.h =====

// One entry per distinct bit layout in the ISA doc.
enum class Format {
    R, R_IMM, CMP, R1, BIT,
    BRANCH, BRANCH_NOOP,
    MEM_LOAD, MEM_STORE, MOVE,
    STACK_1, STACK_N_COUNT, STACK_N_NOOP,
    FP_R, FP_CMP, FP_R1,
    VEC_3REG, VEC_LOAD, VEC_STORE, VEC_SCALAR2, VEC_SCALAR1,
    NO_OPERAND,
    R1S   // NEW: single-register in-place format (incr/decr/abs only)
          // CMP and MOVE keep their existing enum tags — the dispatcher now
          // branches internally between register/immediate sub-forms for
          // both, the same way it already does for Format::R.
};

struct OpcodeInfo {
    uint8_t opcode;
    Format format;
    std::string mnemonic;  // used by the disassembler's reverse lookup
};

// mnemonic -> opcode/format, exactly as the ISA doc's opcode table states.
extern const std::unordered_map<std::string, OpcodeInfo> kOpcodeTable;

// opcode value -> info, built once from kOpcodeTable (for the disassembler).
extern const std::unordered_map<uint8_t, OpcodeInfo> kOpcodeByValue;

// Mnemonics whose last operand may be EITHER a register (R form, I=0)
// OR an immediate (R_IMM form, I=1).
extern const std::unordered_map<std::string, uint8_t> kImmCapable;

// Register aliases (cosmetic only): ip=r29, sp=r30, ra=r31.
// Confirm with the team whether this convenience is actually wanted.
extern const std::unordered_map<std::string, int> kRegisterAliases;

// ===== Declarations from bitfield_utils.h =====

// ---------------------------------------------------------------------------
// Operand parsing (shared by the encoder; used by the disassembler for
// formatting register names back out)
// ---------------------------------------------------------------------------

// True if `s` looks like a register operand ("r0".."r31", or an alias).
bool is_register_operand(const std::string& s);

// Parses "r0".."r31" or an alias (ip/sp/ra) into a 0-31 register number.
// Throws std::runtime_error on anything else or out-of-range numbers.
uint8_t parse_register(const std::string& operand);
// Vector operands are written as the first register of the vector: r20 (v1), r23 (v2) or r26 (v3).
// They are encoded as the vector id 0, 1 or 2 that the control unit's microcode expects.
// Other registers are encoded unchanged (and rejected by the simulator at run time).
uint8_t parse_vector_operand(const std::string& operand);
std::string vector_operand_name(uint8_t id);

// Formats a register number back into "rN" text (used by the disassembler).
std::string register_name(uint8_t num);

// Parses a decimal or 0x-hex immediate (optional leading '#'), signed.
int64_t parse_immediate(const std::string& operand);
int64_t parse_mov_immediate(const std::string& operand);

// ---------------------------------------------------------------------------
// Bit-field packing / unpacking
//
// Bit numbering follows the ISA doc's [hi:lo] convention: bit 63 is the MSB
// (leftmost), bit 0 is the LSB (rightmost).
// ---------------------------------------------------------------------------

// Range-checks a signed value against a `bits`-wide field and returns the
// two's-complement bit pattern to pack into the word. Throws on overflow.
uint64_t to_signed_field(int64_t value, int bits);

// Sign-extends a `bits`-wide raw field back to a signed int64_t (decoding).
int64_t from_signed_field(uint64_t raw, int bits);

// Writes `value` into word bits [hi:lo] inclusive.
void pack_field(uint64_t& word, uint64_t value, int hi, int lo);

// Reads word bits [hi:lo] inclusive as an unsigned value.
uint64_t extract_field(uint64_t word, int hi, int lo);



// ===== Declarations from tokenizer.h =====

// Converts raw .asm source lines into structured Tokens.
//
// Syntax conventions (our design choice — confirm/override with the team):
//   - Comments start with ';' and run to end of line
//   - Labels are "identifier:" — alone on a line, or prefixing an
//     instruction on the same line
//   - Operands are comma-separated, whitespace-trimmed
//   - Memory operands are plain comma-separated operands (rd, rs1, offset),
//     NOT bracket notation like "[rs1 + #offset]" — the ISA doc's bracket
//     notation is documentation-only, not a ruling on assembly text.
std::vector<Token> tokenize(const std::vector<std::string>& lines);

// Stack macro preprocessing: "push r1, r2, r3" is rewritten into three
// single-register instructions (push r1 / push r2 / push r3), in the order
// written; "pop r1, r2, r3" likewise. A label on the line stays on the first
// expanded instruction and every expanded instruction keeps the original line
// number for error messages. Call this on the tokenize() result, BEFORE
// build_symbol_table(), so addresses account for the extra instructions.
std::vector<Token> expand_stack_macros(const std::vector<Token>& tokens);

// ===== Declarations from pass1.h =====

// Walks the token stream once, assigning each instruction its byte address
// (every instruction is a fixed 8-byte / 64-bit word) and recording every
// label's resolved address. Mutates each Token's `address` field in place.
std::unordered_map<std::string, uint32_t> build_symbol_table(
    std::vector<Token>& tokens, uint32_t start_address = 0);

// ===== Declarations from format_encoders.h =====

// One function per distinct bit layout confirmed in the ISA doc.
// Each packs opcode + operands into a 64-bit instruction word.

uint64_t encode_R(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t rs2);
uint64_t encode_R_IMM(uint8_t opcode, uint8_t rd, uint8_t rs1, int64_t imm);
uint64_t encode_CMP(uint8_t opcode, uint8_t rs1, uint8_t rs2);

// NEW (confirmed by the final ISA doc): cmp's immediate form.
// op(7) unused(5) rs1(5) unused(5) I=1(1) imm(41, signed)
uint64_t encode_CMP_IMM(uint8_t opcode, uint8_t rs1, int64_t imm);

uint64_t encode_R1(uint8_t opcode, uint8_t rd, uint8_t rs1);

// NEW (confirmed by the final ISA doc): the single-register "in place" format
// used by incr/decr/abs. Unlike R1, there is NO separate rd field at all —
// the slot where rd would sit is genuinely unused padding, not a copy of rs1.
// op(7) unused(5) rs1(5) unused(47)
uint64_t encode_R1S(uint8_t opcode, uint8_t rs1);

uint64_t encode_BIT(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t bit_idx);
uint64_t encode_BRANCH(uint8_t opcode, int64_t offset);
uint64_t encode_BRANCH_NOOP(uint8_t opcode);
uint64_t encode_MEM_LOAD(uint8_t opcode, uint8_t rd_dest, uint8_t rs1_base, int64_t offset);
uint64_t encode_MEM_STORE(uint8_t opcode, uint8_t rd_base, uint8_t rs1_value, int64_t offset);

// CHANGED (confirmed by the final ISA doc): mov is now two distinct formats,
// not one. The old single encode_MOVE() is replaced by these two.
//
// Register form (mov rd, rs2) — NEW, didn't exist before:
//   op(7) rd(5) unused(5) rs2(5) I=0(1) mode(2) unused(39)
uint64_t encode_MOVE_REG(uint8_t opcode, uint8_t rd, uint8_t rs2, uint8_t mode);

// Immediate form (mov rd, #imm) — bit positions shifted vs the old layout:
//   op(7) rd(5) unused(10) I=1(1) mode(2) value(39, signed)
// `mode` is fully defined at the bit level (00 normal / 01 unsigned / 10
// movh); the ASSEMBLY SYNTAX for selecting mode 01/10 is still undecided —
// see exclusions. The dispatcher only ever calls this with mode=0 for now.
uint64_t encode_MOVE_IMM(uint8_t opcode, uint8_t rd, uint8_t mode, int64_t value);

uint64_t encode_STACK_1(uint8_t opcode, uint8_t rd);
uint64_t encode_STACK_N_COUNT(uint8_t opcode, int64_t count);
uint64_t encode_STACK_N_NOOP(uint8_t opcode);
uint64_t encode_FP_R(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t rs2);
uint64_t encode_FP_CMP(uint8_t opcode, uint8_t rs1, uint8_t rs2);
uint64_t encode_VEC_3REG(uint8_t opcode, uint8_t vd, uint8_t vs1, uint8_t vs2);
uint64_t encode_VEC_LOAD(uint8_t opcode, uint8_t vd, uint8_t rs1_base, int64_t offset);
uint64_t encode_VEC_STORE(uint8_t opcode, uint8_t rd_base, uint8_t vs1_value, int64_t offset);
uint64_t encode_VEC_SCALAR2(uint8_t opcode, uint8_t rd_gpr, uint8_t vs1, uint8_t vs2);
uint64_t encode_VEC_SCALAR1(uint8_t opcode, uint8_t rd_gpr, uint8_t vs1);
uint64_t encode_NO_OPERAND(uint8_t opcode);



// ===== Declarations from encoder.h =====

// Encodes a single Token into its 64-bit instruction word, using the
// mnemonic's registered Format to pick the right bit layout.
//
// EXCLUSIONS enforced here (see the chat summary for the full list):
//   - Branching to a LABEL throws — the offset-reference convention is
//     unconfirmed. Only raw numeric offsets are accepted for now.
//   - `mov` only supports the 2-operand (rd, imm) form -> submode is
//     always 0. Submodes 1/2 have no assembly syntax defined yet.
uint64_t encode_instruction(
    const Token& tok,
    const std::unordered_map<std::string, uint32_t>& symbol_table);

// Runs encode_instruction() over every real instruction in the token stream
// (skipping label-only lines), in order. This is the Pass 2 entry point.
std::vector<uint64_t> pass2_encode(
    std::vector<Token>& tokens,
    const std::unordered_map<std::string, uint32_t>& symbol_table);



// ===== Declarations from disassembler.h =====

// Decodes a single 64-bit instruction word back into readable assembly
// text (e.g. "add r3, r1, r2"). This is the exact inverse of the encoder —
// unlike encoding, decoding never needs to resolve a label, so none of the
// encoder's exclusions (branch-offset convention, mov submode syntax) apply
// here: every field is just extracted and printed as-is.
std::string disassemble_one(uint64_t word);




// ===== Declarations used by the command-line tools =====

uint8_t parse_vector_register(const std::string& operand);
void write_hex_file(const std::string& path, const std::vector<uint64_t>& words);
std::vector<std::string> read_source_file(const std::string& path);

// ---------------------------------------------------------------------------
// Single-header layout: everything above is declarations and is safe to include
// from any file. The definitions below are included by default, so the team's
// stand-alone build (g++ -std=c++17 risc201_assembler.cpp -o assembler) keeps working.
// The CMake build defines RISC201_COMMON_NO_IMPLEMENTATION everywhere and compiles the
// definitions exactly once (in the "assembler" library), so several files can share the header.
// ---------------------------------------------------------------------------
#ifndef RISC201_COMMON_NO_IMPLEMENTATION

// ===== Implementation from bitfield_utils.cpp =====
bool is_register_operand(const std::string& s) {
    if (s.empty()) return false;
    if (kRegisterAliases.count(s)) return true;
    if ((s[0] == 'r' || s[0] == 'R') && s.size() > 1) {
        for (size_t i = 1; i < s.size(); ++i)
            if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        return true;
    }
    return false;
}

uint8_t parse_register(const std::string& operand) {
    auto alias_it = kRegisterAliases.find(operand);
    if (alias_it != kRegisterAliases.end())
        return static_cast<uint8_t>(alias_it->second);

    if (operand.size() < 2 || (operand[0] != 'r' && operand[0] != 'R'))
        throw std::runtime_error("Not a register operand: " + operand);

    int num = std::stoi(operand.substr(1));
    if (num < 0 || num > 31)
        throw std::runtime_error("Register out of range (r0-r31): " + operand);
    return static_cast<uint8_t>(num);
}

uint8_t parse_vector_operand(const std::string& operand) {
    const uint8_t r = parse_register(operand);
    if (r == 20) return 0;
    if (r == 23) return 1;
    if (r == 26) return 2;
    return r;   // any other register is encoded as written; the simulator reports it as an invalid vector id when it runs
}

std::string vector_operand_name(uint8_t id) {
    return id <= 2 ? register_name(static_cast<uint8_t>(20 + 3 * id)) : register_name(id);
}
uint8_t parse_vector_register(const std::string& operand) {
    if (operand.size() < 2 ||
        (operand[0] != 'v' && operand[0] != 'V')) {
        throw std::runtime_error(
            "Not a vector register operand: " + operand
        );
    }

    std::string number = operand.substr(1);

    if (number.empty() ||
        !std::all_of(number.begin(), number.end(),
            [](unsigned char c) { return std::isdigit(c); })) {
        throw std::runtime_error(
            "Invalid vector register: " + operand
        );
    }

    int index = std::stoi(number);

    if (index < 0 || index > 31) {
        throw std::runtime_error(
            "Vector register out of range (v0-v31): " + operand
        );
    }

    return static_cast<uint8_t>(index);
}

std::string register_name(uint8_t num) {
    return "r" + std::to_string(num);
}

int64_t parse_immediate(const std::string& operand) {
    std::string s = operand;
    if (!s.empty() && s[0] == '#') s = s.substr(1);
    try {
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            return static_cast<int64_t>(std::stoll(s, nullptr, 16));
        return std::stoll(s, nullptr, 10);
    } catch (...) {
        throw std::runtime_error("Not a valid immediate: " + operand);
    }
}

// mov accepts an integer (#5, #-3, #0x10) or a float literal (#1.5, #-0.5, #2.5e3). A float literal
// loads the 32-bit IEEE-754 bit pattern of the number, so it can feed fadd, vadd and the other
// float instructions directly.
int64_t parse_mov_immediate(const std::string& operand) {
    std::string s = operand;
    if (!s.empty() && s[0] == '#') s = s.substr(1);
    const bool is_hex = s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X');
    if (!is_hex && s.find_first_of(".eE") != std::string::npos) {
        try {
            size_t used = 0;
            float f = std::stof(s, &used);
            if (used != s.size()) throw std::invalid_argument("trailing text");
            uint32_t bits;
            std::memcpy(&bits, &f, sizeof bits);
            return static_cast<int64_t>(bits);      // 32 bits always fit the 39-bit mov immediate
        } catch (...) {
            throw std::runtime_error("Not a valid float literal: " + operand);
        }
    }
    return parse_immediate(operand);
}

struct MemoryOperand {
    uint8_t base;
    int64_t offset;
};
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");

    if (start == std::string::npos) {
        return "";
    }

    size_t end = s.find_last_not_of(" \t\r\n");

    return s.substr(start, end - start + 1);
}

MemoryOperand parse_memory_operand(const std::string& s) {
    size_t open = s.find('[');
    size_t close = s.find(']');

    if (open == std::string::npos ||
        close == std::string::npos ||
        close != s.size() - 1 ||
        close <= open + 1) {
        throw std::runtime_error(
            "Expected memory operand like 4[r2] or -4[r2]"
        );
    }

    std::string offset_text = trim(s.substr(0, open));
    std::string base_text =
        trim(s.substr(open + 1, close - open - 1));

    if (offset_text.empty()) {
        throw std::runtime_error("Memory offset is missing");
    }

    int64_t offset = parse_immediate(offset_text);
    uint8_t base = parse_register(base_text);

    // Signed 42-bit range:
    // -2^41 through 2^41 - 1
    const int64_t min_offset = -(int64_t{1} << 41);
    const int64_t max_offset =  (int64_t{1} << 41) - 1;

    if (offset < min_offset || offset > max_offset) {
        throw std::runtime_error(
            "Memory offset does not fit in signed 42 bits"
        );
    }

    return {base, offset};
}

uint64_t to_signed_field(int64_t value, int bits) {
    int64_t min_val = -(int64_t(1) << (bits - 1));
    int64_t max_val = (int64_t(1) << (bits - 1)) - 1;
    if (value < min_val || value > max_val)
        throw std::runtime_error(
            "Value " + std::to_string(value) + " out of range for a " +
            std::to_string(bits) + "-bit signed field");
    uint64_t mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    return static_cast<uint64_t>(value) & mask;
}

int64_t from_signed_field(uint64_t raw, int bits) {
    uint64_t mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    raw &= mask;
    uint64_t sign_bit = 1ULL << (bits - 1);
    if (raw & sign_bit) {
        return static_cast<int64_t>(raw) - static_cast<int64_t>(1ULL << bits);
    }
    return static_cast<int64_t>(raw);
}

void pack_field(uint64_t& word, uint64_t value, int hi, int lo) {
    int width = hi - lo + 1;
    uint64_t mask = (width >= 64) ? ~0ULL : ((1ULL << width) - 1);
    word |= (value & mask) << lo;
}

uint64_t extract_field(uint64_t word, int hi, int lo) {
    int width = hi - lo + 1;
    uint64_t mask = (width >= 64) ? ~0ULL : ((1ULL << width) - 1);
    return (word >> lo) & mask;
}


// ===== Implementation from opcode_table.cpp =====
const std::unordered_map<std::string, OpcodeInfo> kOpcodeTable = {
    // Arithmetic
    {"add",  {0x00, Format::R,        "add"}},
    {"sub",  {0x01, Format::R,        "sub"}},
    {"mul",  {0x02, Format::R,        "mul"}},
    {"div",  {0x03, Format::R,        "div"}},
    {"mod",  {0x04, Format::R,        "mod"}},
    {"cmp",  {0x05, Format::CMP,      "cmp"}},   // now has an imm form too — see dispatcher
    {"incr", {0x06, Format::R1S,      "incr"}},  // CHANGED from R1 — no rd field anymore
    {"decr", {0x07, Format::R1S,      "decr"}},  // CHANGED from R1
    {"pow",  {0x08, Format::R,        "pow"}},
    {"abs",  {0x09, Format::R1S,      "abs"}},   // CHANGED from R1

    // Logical
    {"and",  {0x10, Format::R,        "and"}},
    {"or",   {0x11, Format::R,        "or"}},
    {"not",  {0x12, Format::R1,       "not"}},
    {"nor",  {0x13, Format::R,        "nor"}},
    {"nand", {0x14, Format::R,        "nand"}},
    {"xor",  {0x15, Format::R,        "xor"}},
    {"lsl",  {0x16, Format::R,        "lsl"}},
    {"lsr",  {0x17, Format::R,        "lsr"}},
    {"asr",  {0x18, Format::R,        "asr"}},
    {"rotl", {0x19, Format::R,        "rotl"}},
    {"rotr", {0x1A, Format::R,        "rotr"}},

    // Bit manipulation
    {"pcnt", {0x20, Format::R1,       "pcnt"}},
    {"tog",  {0x21, Format::BIT,      "tog"}},
    {"sbit", {0x22, Format::BIT,      "sbit"}},
    {"cbit", {0x23, Format::BIT,      "cbit"}},

    // Branch
    {"call", {0x30, Format::BRANCH,      "call"}},
    {"ret",  {0x31, Format::BRANCH_NOOP, "ret"}},
    {"b",    {0x32, Format::BRANCH,      "b"}},
    {"beq",  {0x33, Format::BRANCH,      "beq"}},
    {"bgt",  {0x34, Format::BRANCH,      "bgt"}},
    {"bst",  {0x35, Format::BRANCH,      "bst"}},

    // Memory
    {"ld",   {0x40, Format::MEM_LOAD,  "ld"}},
    {"st",   {0x41, Format::MEM_STORE, "st"}},
    {"mov",  {0x42, Format::MOVE,      "mov"}},   // submode fixed at 0 — see exclusions

    // Stack
    {"push", {0x50, Format::STACK_1,       "push"}},
    {"pop",  {0x51, Format::STACK_1,       "pop"}},
    {"mpop", {0x52, Format::STACK_N_COUNT, "mpop"}},
    {"peek", {0x53, Format::STACK_1,       "peek"}},
    {"clr",  {0x54, Format::STACK_N_NOOP,  "clr"}},

    // Floating point
    {"fadd", {0x60, Format::FP_R,   "fadd"}},
    {"fsub", {0x61, Format::FP_R,   "fsub"}},
    {"fmul", {0x62, Format::FP_R,   "fmul"}},
    {"fdiv", {0x63, Format::FP_R,   "fdiv"}},
    {"fcmp", {0x64, Format::FP_CMP, "fcmp"}},
    {"sqrt", {0x65, Format::FP_R1,  "sqrt"}},
    {"fabs", {0x66, Format::FP_R1,  "fabs"}},
    {"sin",  {0x67, Format::FP_R1,  "sin"}},
    {"cos",  {0x68, Format::FP_R1,  "cos"}},

    // Vector
    {"vadd", {0x70, Format::VEC_3REG,    "vadd"}},
    {"vsub", {0x71, Format::VEC_3REG,    "vsub"}},
    {"vdot", {0x72, Format::VEC_SCALAR2, "vdot"}},
    {"vcrs", {0x73, Format::VEC_3REG,    "vcrs"}},
    {"vmag", {0x74, Format::VEC_SCALAR1, "vmag"}},
    {"vang", {0x75, Format::VEC_SCALAR2, "vang"}},
    {"vld",  {0x76, Format::VEC_LOAD,    "vld"}},
    {"vst",  {0x77, Format::VEC_STORE,   "vst"}},

    // Misc
    {"nop",  {0x7C, Format::NO_OPERAND, "nop"}},
};

const std::unordered_map<uint8_t, OpcodeInfo> kOpcodeByValue = [] {
    std::unordered_map<uint8_t, OpcodeInfo> m;
    for (const auto& [mnemonic, info] : kOpcodeTable) m[info.opcode] = info;
    return m;
}();

const std::unordered_map<std::string, uint8_t> kImmCapable = {
    {"add", 0x00}, {"sub", 0x01}, {"mul", 0x02}, {"div", 0x03},
    {"mod", 0x04}, {"pow", 0x08},
    {"and", 0x10}, {"or", 0x11}, {"nor", 0x13}, {"nand", 0x14},
    {"xor", 0x15}, {"lsl", 0x16}, {"lsr", 0x17}, {"asr", 0x18},
    {"rotl", 0x19}, {"rotr", 0x1A},
};

const std::unordered_map<std::string, int> kRegisterAliases = {
    {"ip", 29}, {"sp", 30}, {"ra", 31},
};


// ===== Implementation from tokenizer.cpp =====
// static std::string trim(const std::string& s) {
//     size_t start = s.find_first_not_of(" \t\r\n");
//     if (start == std::string::npos) return "";
//     size_t end = s.find_last_not_of(" \t\r\n");
//     return s.substr(start, end - start + 1);
// }

static std::vector<std::string> split_operands(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        std::string t = trim(item);
        if (!t.empty()) out.push_back(t);
    }
    return out;
}

std::vector<Token> tokenize(const std::vector<std::string>& lines) {
    std::vector<Token> tokens;
    int line_no = 0;

    for (const auto& raw_line : lines) {
        ++line_no;
        std::string line = raw_line;

        // Strip comments
        size_t comment_pos = line.find(';');
        if (comment_pos != std::string::npos) line = line.substr(0, comment_pos);
        line = trim(line);
        if (line.empty()) continue;  // blank or comment-only line — no token

        Token tok;
        tok.line_number = line_no;

        // Detect label
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            tok.label = trim(line.substr(0, colon_pos));
            line = trim(line.substr(colon_pos + 1));
        }

        if (line.empty()) {
            tokens.push_back(tok);  // label-only line
            continue;
        }

        // Split mnemonic from operands
        size_t space_pos = line.find_first_of(" \t");
        if (space_pos == std::string::npos) {
            tok.mnemonic = line;  // no operands (e.g. "nop", "ret", "clr")
        } else {
            tok.mnemonic = line.substr(0, space_pos);
            std::string operand_str = trim(line.substr(space_pos + 1));
            tok.operands = split_operands(operand_str);
        }

        // Normalize mnemonic to lowercase
        std::transform(tok.mnemonic.begin(), tok.mnemonic.end(), tok.mnemonic.begin(),
                        [](unsigned char c) { return std::tolower(c); });

        tokens.push_back(tok);
    }

    return tokens;
}


std::vector<Token> expand_stack_macros(const std::vector<Token>& tokens) {
    std::vector<Token> out;
    out.reserve(tokens.size());
    auto fail = [](const Token& tok, const std::string& msg) {
        throw std::runtime_error("Line " + std::to_string(tok.line_number) + ": " + msg);
    };
    for (const Token& tok : tokens) {
        const std::string& m = tok.mnemonic;
        const std::vector<std::string>& ops = tok.operands;

        // Team macros: pushm r1, r2, r3  ->  push r1 / push r2 / push r3
        //              popm  r1, r2, r3  ->  pop r3 / pop r2 / pop r1   (reverse order, undoes pushm)
        if (m == "pushm" || m == "popm") {
            if (ops.empty()) fail(tok, m + ": expected at least one register");
            for (const std::string& r : ops)
                if (!is_register_operand(r)) fail(tok, m + ": '" + r + "' is not a register");
            for (size_t i = 0; i < ops.size(); ++i) {
                Token t = tok;
                t.mnemonic = (m == "pushm") ? "push" : "pop";
                t.operands = {(m == "pushm") ? ops[i] : ops[ops.size() - 1 - i]};
                if (i > 0) t.label.clear();
                out.push_back(t);
            }
        // Also accepted: push r1, r2, r3 / pop r1, r2, r3 (expanded in the order written)
        } else if ((m == "push" || m == "pop") && ops.size() > 1) {
            for (size_t i = 0; i < ops.size(); ++i) {
                Token t = tok;
                t.operands = {ops[i]};
                if (i > 0) t.label.clear();
                out.push_back(t);
            }
        } else {
            out.push_back(tok);
        }
    }
    return out;
}

// ===== Implementation from pass1.cpp =====
std::unordered_map<std::string, uint32_t> build_symbol_table(
    std::vector<Token>& tokens, uint32_t start_address)
{
    std::unordered_map<std::string, uint32_t> symbols;
    uint32_t addr = start_address;

    for (auto& tok : tokens) {
        if (!tok.label.empty()) {
            symbols[tok.label] = addr;
        }
        tok.address = addr;
        if (!tok.mnemonic.empty()) {
            addr += 8;  // every instruction is a fixed 64-bit word
        }
    }
    return symbols;
}


// ===== Implementation from format_encoders.cpp =====
uint64_t encode_R(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd,     56, 52);
    pack_field(w, rs1,    51, 47);
    pack_field(w, rs2,    46, 42);
    pack_field(w, 0,      41, 41);   // I = 0 (register form)
    return w;                        // [40:0] unused
}

uint64_t encode_R_IMM(uint8_t opcode, uint8_t rd, uint8_t rs1, int64_t imm) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd,     56, 52);
    pack_field(w, rs1,    51, 47);
    // [46:42] unused
    pack_field(w, 1,      41, 41);   // I = 1 (immediate form)
    pack_field(w, to_signed_field(imm, 41), 40, 0);
    return w;
}

uint64_t encode_CMP(uint8_t opcode, uint8_t rs1, uint8_t rs2) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    // [56:52] unused (no rd)
    pack_field(w, rs1,    51, 47);
    pack_field(w, rs2,    46, 42);
    pack_field(w, 0,      41, 41);   // I = 0 (register form)
    return w;                        // [40:0] unused
}

// NEW: cmp's immediate form.
uint64_t encode_CMP_IMM(uint8_t opcode, uint8_t rs1, int64_t imm) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    // [56:52] unused (no rd)
    pack_field(w, rs1,    51, 47);
    // [46:42] unused (no rs2 — this slot is immediate space in I=1 form)
    pack_field(w, 1,      41, 41);   // I = 1 (immediate form)
    pack_field(w, to_signed_field(imm, 41), 40, 0);
    return w;
}

uint64_t encode_R1(uint8_t opcode, uint8_t rd, uint8_t rs1) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd,     56, 52);
    pack_field(w, rs1,    51, 47);
    return w;                        // [46:0] unused
}

// NEW: single-register "in place" format for incr/decr/abs. No rd field —
// the old rd slot is genuinely unused padding here, not populated at all.
uint64_t encode_R1S(uint8_t opcode, uint8_t rs1) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    // [56:52] unused — deliberately NOT set to rs1's value
    pack_field(w, rs1,    51, 47);
    return w;                        // [46:0] unused
}

uint64_t encode_BIT(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t bit_idx) {
    uint64_t w = 0;
    pack_field(w, opcode,  63, 57);
    pack_field(w, rd,      56, 52);
    pack_field(w, rs1,     51, 47);
    pack_field(w, bit_idx, 46, 42);
    return w;                        // [41:0] unused
}

uint64_t encode_BRANCH(uint8_t opcode, int64_t offset) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    // [56:42] unused
    pack_field(w, to_signed_field(offset, 42), 41, 0);
    return w;
}

uint64_t encode_BRANCH_NOOP(uint8_t opcode) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    return w;                        // [56:0] unused
}

uint64_t encode_MEM_LOAD(uint8_t opcode, uint8_t rd_dest, uint8_t rs1_base, int64_t offset) {
    uint64_t w = 0;
    pack_field(w, opcode,   63, 57);
    pack_field(w, rd_dest,  56, 52);
    pack_field(w, rs1_base, 51, 47);
    // [46:42] unused
    pack_field(w, to_signed_field(offset, 42), 41, 0);
    return w;
}

uint64_t encode_MEM_STORE(uint8_t opcode, uint8_t rd_base, uint8_t rs1_value, int64_t offset) {
    uint64_t w = 0;
    pack_field(w, opcode,    63, 57);
    pack_field(w, rd_base,   56, 52);
    pack_field(w, rs1_value, 51, 47);
    // [46:42] unused
    pack_field(w, to_signed_field(offset, 42), 41, 0);
    return w;
}

// CHANGED: mov is now two formats (register form and immediate form),
// replacing the old single encode_MOVE(). Bit positions also shifted —
// value is now 39 bits (was 40), and there's a real I-bit at [41:41].

// mov rd, rs2  (I=0): op(7) rd(5) unused(5) rs2(5) I=0(1) mode(2) unused(39)
uint64_t encode_MOVE_REG(uint8_t opcode, uint8_t rd, uint8_t rs2, uint8_t mode) {
    uint64_t w = 0;
    pack_field(w, opcode,      63, 57);
    pack_field(w, rd,          56, 52);
    // [51:47] unused (no rs1 in this form)
    pack_field(w, rs2,         46, 42);
    pack_field(w, 0,           41, 41);   // I = 0 (register form)
    pack_field(w, mode & 0x3,  40, 39);
    return w;                             // [38:0] unused
}

// mov rd, #imm (I=1): op(7) rd(5) unused(10) I=1(1) mode(2) value(39, signed)
uint64_t encode_MOVE_IMM(uint8_t opcode, uint8_t rd, uint8_t mode, int64_t value) {
    uint64_t w = 0;
    pack_field(w, opcode,       63, 57);
    pack_field(w, rd,           56, 52);
    // [51:42] unused (10 bits)
    pack_field(w, 1,            41, 41);  // I = 1 (immediate form)
    pack_field(w, mode & 0x3,   40, 39);
    pack_field(w, to_signed_field(value, 39), 38, 0);
    return w;
}

uint64_t encode_STACK_1(uint8_t opcode, uint8_t rd) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd,     56, 52);
    return w;                        // [51:0] unused
}

uint64_t encode_STACK_N_COUNT(uint8_t opcode, int64_t count) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    // [56:42] unused
    pack_field(w, to_signed_field(count, 42), 41, 0);
    return w;
}

uint64_t encode_STACK_N_NOOP(uint8_t opcode) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    return w;                        // [56:0] unused
}

uint64_t encode_FP_R(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd,     56, 52);
    pack_field(w, rs1,    51, 47);
    pack_field(w, rs2,    46, 42);
    return w;                        // [41:0] unused — no I-bit for FP
}

uint64_t encode_FP_CMP(uint8_t opcode, uint8_t rs1, uint8_t rs2) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    // [56:52] unused
    pack_field(w, rs1,    51, 47);
    pack_field(w, rs2,    46, 42);
    return w;                        // [41:0] unused
}

uint64_t encode_VEC_3REG(uint8_t opcode, uint8_t vd, uint8_t vs1, uint8_t vs2) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, vd,     56, 52);
    pack_field(w, vs1,    51, 47);
    pack_field(w, vs2,    46, 42);
    return w;                        // [41:0] unused
}

uint64_t encode_VEC_LOAD(uint8_t opcode, uint8_t vd, uint8_t rs1_base, int64_t offset) {
    uint64_t w = 0;
    pack_field(w, opcode,   63, 57);
    pack_field(w, vd,       56, 52);
    pack_field(w, rs1_base, 51, 47);
    // [46:42] unused
    pack_field(w, to_signed_field(offset, 42), 41, 0);
    return w;
}

uint64_t encode_VEC_STORE(uint8_t opcode, uint8_t rd_base, uint8_t vs1_value, int64_t offset) {
    uint64_t w = 0;
    pack_field(w, opcode,    63, 57);
    pack_field(w, rd_base,   56, 52);
    pack_field(w, vs1_value, 51, 47);
    // [46:42] unused
    pack_field(w, to_signed_field(offset, 42), 41, 0);
    return w;
}

uint64_t encode_VEC_SCALAR2(uint8_t opcode, uint8_t rd_gpr, uint8_t vs1, uint8_t vs2) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd_gpr, 56, 52);
    pack_field(w, vs1,    51, 47);
    pack_field(w, vs2,    46, 42);
    return w;                        // [41:0] unused
}

uint64_t encode_VEC_SCALAR1(uint8_t opcode, uint8_t rd_gpr, uint8_t vs1) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd_gpr, 56, 52);
    pack_field(w, vs1,    51, 47);
    return w;                        // [46:0] unused
}

uint64_t encode_NO_OPERAND(uint8_t opcode) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    return w;                        // [56:0] unused
}

// ===== Implementation from encoder.cpp =====
uint64_t encode_instruction(
    const Token& tok,
    const std::unordered_map<std::string, uint32_t>& symbol_table)
{
    const std::string& mnemonic = tok.mnemonic;
    auto op_it = kOpcodeTable.find(mnemonic);
    if (op_it == kOpcodeTable.end())
        throw std::runtime_error("Unknown mnemonic: " + tok.mnemonic);

    uint8_t opcode = op_it->second.opcode;
    Format fmt = op_it->second.format;
    const auto& ops = tok.operands;

    switch (fmt) {

        case Format::R: {
            auto imm_it = kImmCapable.find(mnemonic);
            bool last_is_reg = !ops.empty() && is_register_operand(ops.back());
            if (imm_it != kImmCapable.end() && !last_is_reg) {
                if (ops.size() != 3) throw std::runtime_error(mnemonic + ": expected rd, rs1, imm");
                return encode_R_IMM(opcode, parse_register(ops[0]), parse_register(ops[1]),
                                     parse_immediate(ops[2]));
            } else {
                if (ops.size() != 3) throw std::runtime_error(mnemonic + ": expected rd, rs1, rs2");
                return encode_R(opcode, parse_register(ops[0]), parse_register(ops[1]),
                                 parse_register(ops[2]));
            }
        }

        case Format::CMP: {
            // CHANGED: cmp now has a confirmed immediate form (was previously
            // excluded as register-only pending ISA confirmation).
            if (ops.size() != 2) throw std::runtime_error("cmp: expected rs1, rs2/imm");
            bool last_is_reg = is_register_operand(ops[1]);
            if (last_is_reg) {
                return encode_CMP(opcode, parse_register(ops[0]), parse_register(ops[1]));
            } else {
                return encode_CMP_IMM(opcode, parse_register(ops[0]), parse_immediate(ops[1]));
            }
        }

       case Format::R1: {
          if (mnemonic == "not") {
        // Team syntax: not rd, rs1.  The one-operand form (not rd = in place) is still accepted.
        if (ops.size() != 1 && ops.size() != 2)
            throw std::runtime_error("not: expected rd, rs1");

        uint8_t rd = parse_register(ops[0]);
        uint8_t rs1 = ops.size() == 2 ? parse_register(ops[1]) : rd;
        return encode_R1(opcode, rd, rs1);
    }

    if (mnemonic == "pcnt") {
        if (ops.size() != 2)
            throw std::runtime_error("pcnt: expected rd, rs1");

        return encode_R1(
            opcode,
            parse_register(ops[0]),
            parse_register(ops[1])
        );
    }

    // Keep existing behavior for other R1 instructions.
    if (ops.size() == 1) {
        uint8_t rd = parse_register(ops[0]);
        return encode_R1(opcode, rd, rd);
    }

    if (ops.size() == 2) {
        return encode_R1(
            opcode,
            parse_register(ops[0]),
            parse_register(ops[1])
        );
    }

    throw std::runtime_error(mnemonic + ": expected rd [, rs1]");
}
        case Format::R1S: {
            // NEW format: incr/decr/abs are genuinely single-register
            // in-place now — there is no rd field to receive a second operand.
            if (ops.size() != 1) throw std::runtime_error(mnemonic + ": expected a single register (rs1)");
            return encode_R1S(opcode, parse_register(ops[0]));
        }

        case Format::BIT: {
            if (ops.size() != 3) throw std::runtime_error(mnemonic + ": expected rd, rs1, #bit_index");
            int64_t bit_index = parse_immediate(ops[2]);

if (bit_index < 0 || bit_index > 31) {
    throw std::runtime_error(
        mnemonic + ": bit index must be between 0 and 31"
    );
}

return encode_BIT(opcode, parse_register(ops[0]), parse_register(ops[1]),
                  static_cast<uint8_t>(bit_index));
        }

       case Format::BRANCH: {
    // RESOLVED: offset is relative to the branch instruction's OWN
    // address (confirmed by A) — NOT the next instruction (PC+8).
    if (ops.size() != 1) throw std::runtime_error(mnemonic + ": expected a label or #offset");
    auto sym_it = symbol_table.find(ops[0]);
    int64_t target_offset;
    if (sym_it != symbol_table.end()) {
        target_offset = static_cast<int64_t>(sym_it->second) - static_cast<int64_t>(tok.address);
    } else {
        target_offset = parse_immediate(ops[0]);  // raw numeric offset, not a label
    }
    return encode_BRANCH(opcode, target_offset);
}

        case Format::BRANCH_NOOP:
            return encode_BRANCH_NOOP(opcode);

       case Format::MEM_LOAD: {
    if (ops.size() != 2) {
        throw std::runtime_error(
            "ld: expected rd, offset[base]"
        );
    }

    MemoryOperand mem = parse_memory_operand(ops[1]);

    return encode_MEM_LOAD(
        opcode,
        parse_register(ops[0]),
        mem.base,
        mem.offset
    );
}

       case Format::MEM_STORE: {
    if (ops.size() != 2) {
        throw std::runtime_error(
            "st: expected rs1, offset[base]"
        );
    }

    MemoryOperand mem = parse_memory_operand(ops[1]);

    return encode_MEM_STORE(
        opcode,
        mem.base,
        parse_register(ops[0]),
        mem.offset
    );
}

        case Format::MOVE: {
            // CHANGED: mov now dispatches between a register form (mov rd,
            // rs2 — new, didn't exist before) and an immediate form, the
            // same way Format::R dispatches R vs R_IMM.
            // EXCLUSION still stands for the immediate form: only mode 0
            // (normal) is reachable — the assembly syntax for selecting
            // mode 01 (unsigned) / 10 (movh) is still undecided.
            if (ops.size() != 2) throw std::runtime_error("mov: expected rd, rs2  OR  rd, #imm");
            bool last_is_reg = is_register_operand(ops[1]);
            if (last_is_reg) {
                return encode_MOVE_REG(opcode, parse_register(ops[0]), parse_register(ops[1]), /*mode=*/0);
            } else {
                return encode_MOVE_IMM(opcode, parse_register(ops[0]), /*mode=*/0, parse_mov_immediate(ops[1]));
            }
        }

        case Format::STACK_1: {
            if (ops.size() != 1) throw std::runtime_error(mnemonic + ": expected rd");
            return encode_STACK_1(opcode, parse_register(ops[0]));
        }

        case Format::STACK_N_COUNT: {
            if (ops.size() != 1) throw std::runtime_error("mpop: expected #count");
            return encode_STACK_N_COUNT(opcode, parse_immediate(ops[0]));
        }

        case Format::STACK_N_NOOP:
    if (!ops.empty()) {
        throw std::runtime_error(mnemonic + ": expected no operands");
    }
    return encode_STACK_N_NOOP(opcode);

        case Format::FP_R: {
            if (ops.size() != 3) throw std::runtime_error(mnemonic + ": expected rd, rs1, rs2");
            return encode_FP_R(opcode, parse_register(ops[0]), parse_register(ops[1]),
                                parse_register(ops[2]));
        }

        case Format::FP_CMP: {
            if (ops.size() != 2) throw std::runtime_error("fcmp: expected rs1, rs2");
            return encode_FP_CMP(opcode, parse_register(ops[0]), parse_register(ops[1]));
        }

        case Format::FP_R1: {
            if (ops.size() != 2) throw std::runtime_error(mnemonic + ": expected rd, rs1");
            return encode_R1(opcode, parse_register(ops[0]), parse_register(ops[1]));
        }

        case Format::VEC_3REG: {
            if (ops.size() != 3) throw std::runtime_error(mnemonic + ": expected vd, vs1, vs2");
            return encode_VEC_3REG(opcode, parse_vector_operand(ops[0]), parse_vector_operand(ops[1]),
                                    parse_vector_operand(ops[2]));
        }

        case Format::VEC_LOAD: {
            if (ops.size() == 3) {
                // Existing syntax: vld r20, r2, #8
                return encode_VEC_LOAD(
                    opcode,
                    parse_vector_operand(ops[0]),
                    parse_register(ops[1]),
                    parse_immediate(ops[2])
                );
            }

            if (ops.size() == 2) {
                // Address syntax: vld r20, [r2 + #8] or vld r20, 12[r5]
                std::string addr = ops[1];
                addr.erase(
                    std::remove_if(addr.begin(), addr.end(),
                        [](unsigned char c) { return std::isspace(c); }),
                    addr.end()
                );

                std::size_t open = addr.find('[');
                std::size_t close = addr.find(']');
                if (open == std::string::npos || close == std::string::npos ||
                    close <= open || close != addr.size() - 1) {
                    throw std::runtime_error(
                        "vld: expected [base + #offset] or offset[base]"
                    );
                }

                std::string base;
                std::string offset;
                if (open == 0) {
                    // [r2+#8]
                    std::string inside = addr.substr(1, close - 1);
                    std::size_t plus = inside.find('+');
                    if (plus == std::string::npos) {
                        throw std::runtime_error("vld: expected [base + #offset]");
                    }
                    base = inside.substr(0, plus);
                    offset = inside.substr(plus + 1);
                } else {
                    // 12[r5]
                    offset = addr.substr(0, open);
                    base = addr.substr(open + 1, close - open - 1);
                }

                if (!offset.empty() && offset.front() == '#') {
                    offset.erase(0, 1);
                }

                return encode_VEC_LOAD(
                    opcode,
                    parse_vector_operand(ops[0]),
                    parse_register(base),
                    parse_immediate(offset)
                );
            }

            throw std::runtime_error(
                "vld: expected vd, rs1, #offset OR vd, [rs1 + #offset]"
            );
        }

        case Format::VEC_STORE: {
            if (ops.size() == 3) {
                // Existing syntax: vst r20, r2, #8
                return encode_VEC_STORE(
                    opcode,
                    parse_register(ops[1]),
                    parse_vector_operand(ops[0]),
                    parse_immediate(ops[2])
                );
            }

            if (ops.size() == 2) {
                // Address syntax: vst r20, [r2 + #8] or vst r20, 12[r5]
                std::string addr = ops[1];
                addr.erase(
                    std::remove_if(addr.begin(), addr.end(),
                        [](unsigned char c) { return std::isspace(c); }),
                    addr.end()
                );

                std::size_t open = addr.find('[');
                std::size_t close = addr.find(']');
                if (open == std::string::npos || close == std::string::npos ||
                    close <= open || close != addr.size() - 1) {
                    throw std::runtime_error(
                        "vst: expected [base + #offset] or offset[base]"
                    );
                }

                std::string base;
                std::string offset;
                if (open == 0) {
                    std::string inside = addr.substr(1, close - 1);
                    std::size_t plus = inside.find('+');
                    if (plus == std::string::npos) {
                        throw std::runtime_error("vst: expected [base + #offset]");
                    }
                    base = inside.substr(0, plus);
                    offset = inside.substr(plus + 1);
                } else {
                    offset = addr.substr(0, open);
                    base = addr.substr(open + 1, close - open - 1);
                }

                if (!offset.empty() && offset.front() == '#') {
                    offset.erase(0, 1);
                }

                return encode_VEC_STORE(
                    opcode,
                    parse_register(base),
                    parse_vector_operand(ops[0]),
                    parse_immediate(offset)
                );
            }

            throw std::runtime_error("vst: expected source register and address");
        }

        case Format::VEC_SCALAR2: {
            if (ops.size() != 3) throw std::runtime_error(mnemonic + ": expected rd, vs1, vs2");
            return encode_VEC_SCALAR2(opcode, parse_register(ops[0]), parse_vector_operand(ops[1]),
                                       parse_vector_operand(ops[2]));
        }

        case Format::VEC_SCALAR1: {
            if (ops.size() != 2) throw std::runtime_error("vmag: expected rd, vs1");
            return encode_VEC_SCALAR1(opcode, parse_register(ops[0]), parse_vector_operand(ops[1]));
        }

        case Format::NO_OPERAND:
    if (!ops.empty()) {
        throw std::runtime_error(mnemonic + ": expected no operands");
    }
    return encode_NO_OPERAND(opcode);
    
        case Format::R_IMM:
            throw std::runtime_error("Internal error: R_IMM should not be dispatched directly");
    }
    throw std::runtime_error("Unhandled format for mnemonic: " + mnemonic);
}

std::vector<uint64_t> pass2_encode(
    std::vector<Token>& tokens,
    const std::unordered_map<std::string, uint32_t>& symbol_table)
{
    std::vector<uint64_t> words;
    for (const auto& tok : tokens) {
        if (tok.mnemonic.empty()) continue;
        words.push_back(encode_instruction(tok, symbol_table));
    }
    return words;
}

// ===== Implementation from disassembler.cpp =====
std::string disassemble_one(uint64_t word) {
    uint8_t opcode = static_cast<uint8_t>(extract_field(word, 63, 57));
    auto it = kOpcodeByValue.find(opcode);
    if (it == kOpcodeByValue.end()) {
        std::ostringstream oss;
        oss << "??? (unknown opcode 0x" << std::hex << (int)opcode << ")";
        return oss.str();
    }
    const std::string& m = it->second.mnemonic;
    Format fmt = it->second.format;
    std::ostringstream oss;
    oss << m;

    switch (fmt) {
        case Format::R:
        case Format::R_IMM: {
            uint8_t rd  = extract_field(word, 56, 52);
            uint8_t rs1 = extract_field(word, 51, 47);
            uint8_t I   = extract_field(word, 41, 41);
            if (I == 0) {
                uint8_t rs2 = extract_field(word, 46, 42);
                oss << " " << register_name(rd) << ", " << register_name(rs1)
                    << ", " << register_name(rs2);
            } else {
                int64_t imm = from_signed_field(extract_field(word, 40, 0), 41);
                oss << " " << register_name(rd) << ", " << register_name(rs1)
                    << ", #" << imm;
            }
            break;
        }
        case Format::CMP: {
            // CHANGED: cmp now has a confirmed I-bit, same dispatch as R/R_IMM.
            uint8_t rs1 = extract_field(word, 51, 47);
            uint8_t I   = extract_field(word, 41, 41);
            if (I == 0) {
                uint8_t rs2 = extract_field(word, 46, 42);
                oss << " " << register_name(rs1) << ", " << register_name(rs2);
            } else {
                int64_t imm = from_signed_field(extract_field(word, 40, 0), 41);
                oss << " " << register_name(rs1) << ", #" << imm;
            }
            break;
        }
        case Format::R1: {
            uint8_t rd  = extract_field(word, 56, 52);
            uint8_t rs1 = extract_field(word, 51, 47);
            if (rd == rs1 && m != "not" && m != "pcnt") oss << " " << register_name(rd);
            else oss << " " << register_name(rd) << ", " << register_name(rs1);
            break;
        }
        case Format::R1S: {
            // NEW: single-register format for incr/decr/abs — no rd field.
            uint8_t rs1 = extract_field(word, 51, 47);
            oss << " " << register_name(rs1);
            break;
        }
        case Format::BIT: {
            uint8_t rd  = extract_field(word, 56, 52);
            uint8_t rs1 = extract_field(word, 51, 47);
            uint8_t idx = extract_field(word, 46, 42);
            oss << " " << register_name(rd) << ", " << register_name(rs1) << ", #" << (int)idx;
            break;
        }
        case Format::BRANCH: {
            int64_t offset = from_signed_field(extract_field(word, 41, 0), 42);
            oss << " #" << offset;
            break;
        }
        case Format::BRANCH_NOOP:
        case Format::STACK_N_NOOP:
        case Format::NO_OPERAND:
            break;  // no operands
        
        case Format::MEM_LOAD: {
            uint8_t rd  = extract_field(word, 56, 52);
            uint8_t rs1 = extract_field(word, 51, 47);
            int64_t offset =
                from_signed_field(extract_field(word, 41, 0), 42);

            oss << " " << register_name(rd)
                << ", " << offset
                << "[" << register_name(rs1) << "]";
            break;
        }

        case Format::MEM_STORE: {
            uint8_t rd_base   = extract_field(word, 56, 52);
            uint8_t rs1_value = extract_field(word, 51, 47);
            int64_t offset =
                from_signed_field(extract_field(word, 41, 0), 42);

            oss << " " << register_name(rs1_value)
                << ", " << offset
                << "[" << register_name(rd_base) << "]";
            break;
        }

        case Format::MOVE: {
            // CHANGED: mov now dispatches on I-bit between register form
            // (mov rd, rs2) and immediate form, with mode at [40:39] in
            // both and a 39-bit (not 40-bit) value field in the imm form.
            uint8_t rd = extract_field(word, 56, 52);
            uint8_t I  = extract_field(word, 41, 41);
            if (I == 0) {
                uint8_t rs2  = extract_field(word, 46, 42);
                uint8_t mode = extract_field(word, 40, 39);
                oss << " " << register_name(rd) << ", " << register_name(rs2);
                if (mode != 0) oss << "  ; mode=" << (int)mode << " (unsigned/movh — syntax TBD)";
            } else {
                uint8_t mode = extract_field(word, 40, 39);
                int64_t value = from_signed_field(extract_field(word, 38, 0), 39);
                oss << " " << register_name(rd) << ", #" << value;
                if (mode != 0) oss << "  ; mode=" << (int)mode << " (unsigned/movh — syntax TBD)";
            }
            break;
        }
        case Format::STACK_1: {
            uint8_t rd = extract_field(word, 56, 52);
            oss << " " << register_name(rd);
            break;
        }
        case Format::STACK_N_COUNT: {
            int64_t count = from_signed_field(extract_field(word, 41, 0), 42);
            oss << " #" << count;
            break;
        }
        case Format::FP_R: {
            uint8_t rd  = extract_field(word, 56, 52);
            uint8_t rs1 = extract_field(word, 51, 47);
            uint8_t rs2 = extract_field(word, 46, 42);
            oss << " " << register_name(rd) << ", " << register_name(rs1) << ", " << register_name(rs2);
            break;
        }
        case Format::FP_CMP: {
            uint8_t rs1 = extract_field(word, 51, 47);
            uint8_t rs2 = extract_field(word, 46, 42);
            oss << " " << register_name(rs1) << ", " << register_name(rs2);
            break;
        }
        case Format::FP_R1: {
            uint8_t rd  = extract_field(word, 56, 52);
            uint8_t rs1 = extract_field(word, 51, 47);
            oss << " " << register_name(rd) << ", " << register_name(rs1);
            break;
        }
        case Format::VEC_3REG: {
            uint8_t vd  = extract_field(word, 56, 52);
            uint8_t vs1 = extract_field(word, 51, 47);
            uint8_t vs2 = extract_field(word, 46, 42);
            oss << " " << vector_operand_name(vd) << ", " << vector_operand_name(vs1) << ", " << vector_operand_name(vs2);
            break;
        }
        case Format::VEC_LOAD: {
            uint8_t vd  = extract_field(word, 56, 52);
            uint8_t rs1 = extract_field(word, 51, 47);
            int64_t offset = from_signed_field(extract_field(word, 41, 0), 42);
            oss << " " << vector_operand_name(vd) << ", " << register_name(rs1) << ", #" << offset;
            break;
        }
        case Format::VEC_STORE: {
            uint8_t rd_base   = extract_field(word, 56, 52);
            uint8_t vs1_value = extract_field(word, 51, 47);
            int64_t offset = from_signed_field(extract_field(word, 41, 0), 42);
            oss << " " << vector_operand_name(vs1_value) << ", " << register_name(rd_base) << ", #" << offset;
            break;
        }
        case Format::VEC_SCALAR2: {
            uint8_t rd  = extract_field(word, 56, 52);
            uint8_t vs1 = extract_field(word, 51, 47);
            uint8_t vs2 = extract_field(word, 46, 42);
            oss << " " << register_name(rd) << ", " << vector_operand_name(vs1) << ", " << vector_operand_name(vs2);
            break;
        }
        case Format::VEC_SCALAR1: {
            uint8_t rd  = extract_field(word, 56, 52);
            uint8_t vs1 = extract_field(word, 51, 47);
            oss << " " << register_name(rd) << ", " << vector_operand_name(vs1);
            break;
        }
    }
    return oss.str();
}


// ===== Implementation from main.cpp =====
// ---------------------------------------------------------------------------
// Phase 1 output format: plain hex text.
//
// One 64-bit instruction word per line, written as 16 lowercase hex digits,
// no "0x" prefix, no addresses, one line per instruction in program order.
// This is OUR choice of hex convention -- confirm with E (simulator/CLI
// owner) that this is the exact format their loader expects before relying
// on it; a different convention (0x-prefixed, space-separated bytes,
// addresses included, etc.) is an easy change here if needed.
// ---------------------------------------------------------------------------
void write_hex_file(const std::string& path, const std::vector<uint64_t>& words) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Could not open output file for writing: " + path);
    }
    for (uint64_t w : words) {
        out << std::hex << std::setw(16) << std::setfill('0') << w << "\n";
    }
}

std::vector<std::string> read_source_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open input file: " + path);
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    return lines;
}

// Built-in demo program, used only when no input file is given on the
// command line -- kept so `./risc201` with no arguments still demonstrates
// the pipeline end to end.
[[maybe_unused]] static std::vector<std::string> demo_source() {
    return {
        "; RISC201 sample program",
        "mov r1, 5",
        "mov r2, 10",
        "add r3, r1, r2      ; register form (I=0)",
        "add r4, r3, 1       ; immediate form (I=1)",
        "incr r4             ; R1S: single-register in place, no rd field",
        "push r4",
        "cmp r4, r2",
        "b #16               ; raw offset -- label branching not yet supported",
        "ld r5, r30, 0",
        "vadd r26, r20, r23   ; v3 = v1 + v2 (a vector operand is r20, r23 or r26)",
        "nop",
        "decr r6",
        "abs r6",
        "cmp r4, 7           ; CMP-IMM: cmp now supports an immediate",
        "mov r8, r9          ; MOVE register form",
        "sub r1, r2, 7",
        "mul r1, r2, r5",
        "and r1 , r5 , r9",
        "and r2 , r4 , 9",
        "mul r1 , r3 , 5",
    };
}

#endif  // !RISC201_COMMON_NO_IMPLEMENTATION
