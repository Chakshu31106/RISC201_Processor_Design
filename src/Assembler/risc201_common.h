#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

struct Token {
    std::string label;
    std::string mnemonic;
    std::vector<std::string> operands;
    uint32_t address = 0;
    int line_number = 0;
};

enum class Format {
    R, R_IMM, CMP, R1, BIT,
    BRANCH, BRANCH_NOOP,
    MEM_LOAD, MEM_STORE, MOVE,
    STACK_1, STACK_N_COUNT, STACK_N_NOOP,
    FP_R, FP_CMP, FP_R1,
    VEC_3REG, VEC_LOAD, VEC_STORE, VEC_SCALAR2, VEC_SCALAR1,
    NO_OPERAND,
    R1S
};

struct OpcodeInfo {
    uint8_t opcode;
    Format format;
    std::string mnemonic;
};

extern const std::unordered_map<std::string, OpcodeInfo> kOpcodeTable;

extern const std::unordered_map<uint8_t, OpcodeInfo> kOpcodeByValue;

extern const std::unordered_map<std::string, uint8_t> kImmCapable;

extern const std::unordered_map<std::string, int> kRegisterAliases;

bool is_register_operand(const std::string& s);

uint8_t parse_register(const std::string& operand);

std::string register_name(uint8_t num);

int64_t parse_immediate(const std::string& operand);

uint64_t to_signed_field(int64_t value, int bits);

int64_t from_signed_field(uint64_t raw, int bits);

void pack_field(uint64_t& word, uint64_t value, int hi, int lo);

uint64_t extract_field(uint64_t word, int hi, int lo);

std::vector<Token> tokenize(const std::vector<std::string>& lines);

std::unordered_map<std::string, uint32_t> build_symbol_table(
    std::vector<Token>& tokens, uint32_t start_address = 0);

uint64_t encode_R(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t rs2);
uint64_t encode_R_IMM(uint8_t opcode, uint8_t rd, uint8_t rs1, int64_t imm);
uint64_t encode_CMP(uint8_t opcode, uint8_t rs1, uint8_t rs2);

uint64_t encode_CMP_IMM(uint8_t opcode, uint8_t rs1, int64_t imm);

uint64_t encode_R1(uint8_t opcode, uint8_t rd, uint8_t rs1);

uint64_t encode_R1S(uint8_t opcode, uint8_t rs1);

uint64_t encode_BIT(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t bit_idx);
uint64_t encode_BRANCH(uint8_t opcode, int64_t offset);
uint64_t encode_BRANCH_NOOP(uint8_t opcode);
uint64_t encode_MEM_LOAD(uint8_t opcode, uint8_t rd_dest, uint8_t rs1_base, int64_t offset);
uint64_t encode_MEM_STORE(uint8_t opcode, uint8_t rd_base, uint8_t rs1_value, int64_t offset);

uint64_t encode_MOVE_REG(uint8_t opcode, uint8_t rd, uint8_t rs2, uint8_t mode);

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

uint64_t encode_instruction(
    const Token& tok,
    const std::unordered_map<std::string, uint32_t>& symbol_table);

std::vector<uint64_t> pass2_encode(
    std::vector<Token>& tokens,
    const std::unordered_map<std::string, uint32_t>& symbol_table);

std::string disassemble_one(uint64_t word);

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

const std::unordered_map<std::string, OpcodeInfo> kOpcodeTable = {
    {"add",  {0x00, Format::R,        "add"}},
    {"sub",  {0x01, Format::R,        "sub"}},
    {"mul",  {0x02, Format::R,        "mul"}},
    {"div",  {0x03, Format::R,        "div"}},
    {"mod",  {0x04, Format::R,        "mod"}},
    {"cmp",  {0x05, Format::CMP,      "cmp"}},
    {"incr", {0x06, Format::R1S,      "incr"}},
    {"decr", {0x07, Format::R1S,      "decr"}},
    {"pow",  {0x08, Format::R,        "pow"}},
    {"abs",  {0x09, Format::R1S,      "abs"}},

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

    {"pcnt", {0x20, Format::R1,       "pcnt"}},
    {"tog",  {0x21, Format::BIT,      "tog"}},
    {"sbit", {0x22, Format::BIT,      "sbit"}},
    {"cbit", {0x23, Format::BIT,      "cbit"}},

    {"call", {0x30, Format::BRANCH,      "call"}},
    {"ret",  {0x31, Format::BRANCH_NOOP, "ret"}},
    {"b",    {0x32, Format::BRANCH,      "b"}},
    {"beq",  {0x33, Format::BRANCH,      "beq"}},
    {"bgt",  {0x34, Format::BRANCH,      "bgt"}},
    {"bst",  {0x35, Format::BRANCH,      "bst"}},

    {"ld",   {0x40, Format::MEM_LOAD,  "ld"}},
    {"st",   {0x41, Format::MEM_STORE, "st"}},
    {"mov",  {0x42, Format::MOVE,      "mov"}},

    {"push", {0x50, Format::STACK_1,       "push"}},
    {"pop",  {0x51, Format::STACK_1,       "pop"}},
    {"mpop", {0x52, Format::STACK_N_COUNT, "mpop"}},
    {"peek", {0x53, Format::STACK_1,       "peek"}},
    {"clr",  {0x54, Format::STACK_N_NOOP,  "clr"}},

    {"fadd", {0x60, Format::FP_R,   "fadd"}},
    {"fsub", {0x61, Format::FP_R,   "fsub"}},
    {"fmul", {0x62, Format::FP_R,   "fmul"}},
    {"fdiv", {0x63, Format::FP_R,   "fdiv"}},
    {"fcmp", {0x64, Format::FP_CMP, "fcmp"}},
    {"sqrt", {0x65, Format::FP_R1,  "sqrt"}},
    {"fabs", {0x66, Format::FP_R1,  "fabs"}},
    {"sin",  {0x67, Format::FP_R1,  "sin"}},
    {"cos",  {0x68, Format::FP_R1,  "cos"}},

    {"vadd", {0x70, Format::VEC_3REG,    "vadd"}},
    {"vsub", {0x71, Format::VEC_3REG,    "vsub"}},
    {"vdot", {0x72, Format::VEC_SCALAR2, "vdot"}},
    {"vcrs", {0x73, Format::VEC_3REG,    "vcrs"}},
    {"vmag", {0x74, Format::VEC_SCALAR1, "vmag"}},
    {"vang", {0x75, Format::VEC_SCALAR2, "vang"}},
    {"vld",  {0x76, Format::VEC_LOAD,    "vld"}},
    {"vst",  {0x77, Format::VEC_STORE,   "vst"}},

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

        size_t comment_pos = line.find(';');
        if (comment_pos != std::string::npos) line = line.substr(0, comment_pos);
        line = trim(line);
        if (line.empty()) continue;

        Token tok;
        tok.line_number = line_no;

        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            tok.label = trim(line.substr(0, colon_pos));
            line = trim(line.substr(colon_pos + 1));
        }

        if (line.empty()) {
            tokens.push_back(tok);
            continue;
        }

        size_t space_pos = line.find_first_of(" \t");
        if (space_pos == std::string::npos) {
            tok.mnemonic = line;
        } else {
            tok.mnemonic = line.substr(0, space_pos);
            std::string operand_str = trim(line.substr(space_pos + 1));
            tok.operands = split_operands(operand_str);
        }

        std::transform(tok.mnemonic.begin(), tok.mnemonic.end(), tok.mnemonic.begin(),
                        [](unsigned char c) { return std::tolower(c); });

        tokens.push_back(tok);
    }

    return tokens;
}

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
            addr += 8;
        }
    }
    return symbols;
}

uint64_t encode_R(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd,     56, 52);
    pack_field(w, rs1,    51, 47);
    pack_field(w, rs2,    46, 42);
    pack_field(w, 0,      41, 41);
    return w;
}

uint64_t encode_R_IMM(uint8_t opcode, uint8_t rd, uint8_t rs1, int64_t imm) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd,     56, 52);
    pack_field(w, rs1,    51, 47);
    pack_field(w, 1,      41, 41);
    pack_field(w, to_signed_field(imm, 41), 40, 0);
    return w;
}

uint64_t encode_CMP(uint8_t opcode, uint8_t rs1, uint8_t rs2) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rs1,    51, 47);
    pack_field(w, rs2,    46, 42);
    pack_field(w, 0,      41, 41);
    return w;
}

uint64_t encode_CMP_IMM(uint8_t opcode, uint8_t rs1, int64_t imm) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rs1,    51, 47);
    pack_field(w, 1,      41, 41);
    pack_field(w, to_signed_field(imm, 41), 40, 0);
    return w;
}

uint64_t encode_R1(uint8_t opcode, uint8_t rd, uint8_t rs1) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd,     56, 52);
    pack_field(w, rs1,    51, 47);
    return w;
}

uint64_t encode_R1S(uint8_t opcode, uint8_t rs1) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rs1,    51, 47);
    return w;
}

uint64_t encode_BIT(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t bit_idx) {
    uint64_t w = 0;
    pack_field(w, opcode,  63, 57);
    pack_field(w, rd,      56, 52);
    pack_field(w, rs1,     51, 47);
    pack_field(w, bit_idx, 46, 42);
    return w;
}

uint64_t encode_BRANCH(uint8_t opcode, int64_t offset) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, to_signed_field(offset, 42), 41, 0);
    return w;
}

uint64_t encode_BRANCH_NOOP(uint8_t opcode) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    return w;
}

uint64_t encode_MEM_LOAD(uint8_t opcode, uint8_t rd_dest, uint8_t rs1_base, int64_t offset) {
    uint64_t w = 0;
    pack_field(w, opcode,   63, 57);
    pack_field(w, rd_dest,  56, 52);
    pack_field(w, rs1_base, 51, 47);
    pack_field(w, to_signed_field(offset, 42), 41, 0);
    return w;
}

uint64_t encode_MEM_STORE(uint8_t opcode, uint8_t rd_base, uint8_t rs1_value, int64_t offset) {
    uint64_t w = 0;
    pack_field(w, opcode,    63, 57);
    pack_field(w, rd_base,   56, 52);
    pack_field(w, rs1_value, 51, 47);
    pack_field(w, to_signed_field(offset, 42), 41, 0);
    return w;
}

uint64_t encode_MOVE_REG(uint8_t opcode, uint8_t rd, uint8_t rs2, uint8_t mode) {
    uint64_t w = 0;
    pack_field(w, opcode,      63, 57);
    pack_field(w, rd,          56, 52);
    pack_field(w, rs2,         46, 42);
    pack_field(w, 0,           41, 41);
    pack_field(w, mode & 0x3,  40, 39);
    return w;
}

uint64_t encode_MOVE_IMM(uint8_t opcode, uint8_t rd, uint8_t mode, int64_t value) {
    uint64_t w = 0;
    pack_field(w, opcode,       63, 57);
    pack_field(w, rd,           56, 52);
    pack_field(w, 1,            41, 41);
    pack_field(w, mode & 0x3,   40, 39);
    pack_field(w, to_signed_field(value, 39), 38, 0);
    return w;
}

uint64_t encode_STACK_1(uint8_t opcode, uint8_t rd) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd,     56, 52);
    return w;
}

uint64_t encode_STACK_N_COUNT(uint8_t opcode, int64_t count) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, to_signed_field(count, 42), 41, 0);
    return w;
}

uint64_t encode_STACK_N_NOOP(uint8_t opcode) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    return w;
}

uint64_t encode_FP_R(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd,     56, 52);
    pack_field(w, rs1,    51, 47);
    pack_field(w, rs2,    46, 42);
    return w;
}

uint64_t encode_FP_CMP(uint8_t opcode, uint8_t rs1, uint8_t rs2) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rs1,    51, 47);
    pack_field(w, rs2,    46, 42);
    return w;
}

uint64_t encode_VEC_3REG(uint8_t opcode, uint8_t vd, uint8_t vs1, uint8_t vs2) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, vd,     56, 52);
    pack_field(w, vs1,    51, 47);
    pack_field(w, vs2,    46, 42);
    return w;
}

uint64_t encode_VEC_LOAD(uint8_t opcode, uint8_t vd, uint8_t rs1_base, int64_t offset) {
    uint64_t w = 0;
    pack_field(w, opcode,   63, 57);
    pack_field(w, vd,       56, 52);
    pack_field(w, rs1_base, 51, 47);
    pack_field(w, to_signed_field(offset, 42), 41, 0);
    return w;
}

uint64_t encode_VEC_STORE(uint8_t opcode, uint8_t rd_base, uint8_t vs1_value, int64_t offset) {
    uint64_t w = 0;
    pack_field(w, opcode,    63, 57);
    pack_field(w, rd_base,   56, 52);
    pack_field(w, vs1_value, 51, 47);
    pack_field(w, to_signed_field(offset, 42), 41, 0);
    return w;
}

uint64_t encode_VEC_SCALAR2(uint8_t opcode, uint8_t rd_gpr, uint8_t vs1, uint8_t vs2) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd_gpr, 56, 52);
    pack_field(w, vs1,    51, 47);
    pack_field(w, vs2,    46, 42);
    return w;
}

uint64_t encode_VEC_SCALAR1(uint8_t opcode, uint8_t rd_gpr, uint8_t vs1) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    pack_field(w, rd_gpr, 56, 52);
    pack_field(w, vs1,    51, 47);
    return w;
}

uint64_t encode_NO_OPERAND(uint8_t opcode) {
    uint64_t w = 0;
    pack_field(w, opcode, 63, 57);
    return w;
}

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
        if (ops.size() != 1)
            throw std::runtime_error("not: expected rd");

        uint8_t rd = parse_register(ops[0]);
        return encode_R1(opcode, rd, rd);
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
    if (ops.size() != 1) throw std::runtime_error(mnemonic + ": expected a label or #offset");
    auto sym_it = symbol_table.find(ops[0]);
    int64_t target_offset;
    if (sym_it != symbol_table.end()) {
        target_offset = static_cast<int64_t>(sym_it->second) - static_cast<int64_t>(tok.address);
    } else {
        target_offset = parse_immediate(ops[0]);
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
            if (ops.size() != 2) throw std::runtime_error("mov: expected rd, rs2  OR  rd, #imm");
            bool last_is_reg = is_register_operand(ops[1]);
            if (last_is_reg) {
                return encode_MOVE_REG(opcode, parse_register(ops[0]), parse_register(ops[1]), 0);
            } else {
                return encode_MOVE_IMM(opcode, parse_register(ops[0]), 0, parse_immediate(ops[1]));
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
            return encode_VEC_3REG(opcode, parse_register(ops[0]), parse_register(ops[1]),
                                    parse_register(ops[2]));
        }

        case Format::VEC_LOAD: {
            if (ops.size() == 3) {
                return encode_VEC_LOAD(
                    opcode,
                    parse_register(ops[0]),
                    parse_register(ops[1]),
                    parse_immediate(ops[2])
                );
            }
            if (ops.size() == 2 &&
    ops[1].find('[') == std::string::npos) {

    return encode_VEC_LOAD(
        opcode,
        parse_register(ops[0]),
        parse_register(ops[1]),
        0
    );
}
            if (ops.size() == 2) {
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
                    std::string inside = addr.substr(1, close - 1);
                    std::size_t plus = inside.find('+');
                    if (plus == std::string::npos) {
                        throw std::runtime_error("vld: expected [base + #offset]");
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

                return encode_VEC_LOAD(
                    opcode,
                    parse_register(ops[0]),
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
                return encode_VEC_STORE(
                    opcode,
                    parse_register(ops[1]),
                    parse_register(ops[0]),
                    parse_immediate(ops[2])
                );
            }

            if (ops.size() == 2) {
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
                    parse_register(ops[0]),
                    parse_immediate(offset)
                );
            }

            throw std::runtime_error("vst: expected source register and address");
        }

        case Format::VEC_SCALAR2: {
            if (ops.size() != 3) throw std::runtime_error(mnemonic + ": expected rd, vs1, vs2");
            return encode_VEC_SCALAR2(opcode, parse_register(ops[0]), parse_register(ops[1]),
                                       parse_register(ops[2]));
        }

        case Format::VEC_SCALAR1: {
            if (ops.size() != 2) throw std::runtime_error("vmag: expected rd, vs1");
            return encode_VEC_SCALAR1(opcode, parse_register(ops[0]), parse_register(ops[1]));
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
            if (rd == rs1) oss << " " << register_name(rd);
            else oss << " " << register_name(rd) << ", " << register_name(rs1);
            break;
        }
        case Format::R1S: {
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
            break;
        
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
            oss << " " << register_name(vd) << ", " << register_name(vs1) << ", " << register_name(vs2);
            break;
        }
        case Format::VEC_LOAD: {
            uint8_t vd  = extract_field(word, 56, 52);
            uint8_t rs1 = extract_field(word, 51, 47);
            int64_t offset = from_signed_field(extract_field(word, 41, 0), 42);
            oss << " " << register_name(vd) << ", " << register_name(rs1) << ", #" << offset;
            break;
        }
        case Format::VEC_STORE: {
            uint8_t rd_base   = extract_field(word, 56, 52);
            uint8_t vs1_value = extract_field(word, 51, 47);
            int64_t offset = from_signed_field(extract_field(word, 41, 0), 42);
            oss << " " << register_name(vs1_value) << ", " << register_name(rd_base) << ", #" << offset;
            break;
        }
        case Format::VEC_SCALAR2: {
            uint8_t rd  = extract_field(word, 56, 52);
            uint8_t vs1 = extract_field(word, 51, 47);
            uint8_t vs2 = extract_field(word, 46, 42);
            oss << " " << register_name(rd) << ", " << register_name(vs1) << ", " << register_name(vs2);
            break;
        }
        case Format::VEC_SCALAR1: {
            uint8_t rd  = extract_field(word, 56, 52);
            uint8_t vs1 = extract_field(word, 51, 47);
            oss << " " << register_name(rd) << ", " << register_name(vs1);
            break;
        }
    }
    return oss.str();
}

static void write_hex_file(const std::string& path, const std::vector<uint64_t>& words) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Could not open output file for writing: " + path);
    }
    for (uint64_t w : words) {
        out << std::hex << std::setw(16) << std::setfill('0') << w << "\n";
    }
}

static std::vector<std::string> read_source_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open input file: " + path);
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    return lines;
}

static std::vector<std::string> demo_source() {
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
        "vadd r20, r23, r26  ; vec0 = vec1 + vec2 (raw register fields)",
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

