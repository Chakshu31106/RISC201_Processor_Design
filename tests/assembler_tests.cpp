#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Simulator/program_loader.h"
#include "Assembler/risc201_common.h"
#include "ISA/hex_file.h"
#include "Simulator/cpu.h"
#include "Simulator/decoder.h"
#include "test_framework.h"
#include "test_helpers.h"

using namespace risc201;



TEST(assemble_assigns_addresses_and_line_numbers) {
    std::ostringstream err;
    AssemblyResult r = assemble({"; comment", "start: mov r1, 5", "", "nop"}, err);
    CHECK(r.ok());
    CHECK_EQ(r.instructions.size(), 2u);
    CHECK_EQ(r.instructions[0].address, 0u);
    CHECK_EQ(r.instructions[0].line_number, 2);
    CHECK_EQ(r.instructions[1].address, 8u);
    CHECK_EQ(r.instructions[1].line_number, 4);
}

TEST(assemble_resolves_labels_relative_to_the_branch) {
    std::ostringstream err;
    // forward: b at 0 -> end at 16  => +16
    AssemblyResult fwd = assemble({"b end", "nop", "end: nop"}, err);
    CHECK(fwd.ok());
    CHECK_EQ(decode(fwd.instructions[0].word).imm, 16);

    // backward: bgt at 24 -> loop at 0 => -24
    AssemblyResult back = assemble({"loop: nop", "nop", "nop", "bgt loop"}, err);
    CHECK(back.ok());
    CHECK_EQ(decode(back.instructions[3].word).imm, -24);
}

TEST(assemble_reports_every_bad_line_and_keeps_going) {
    std::ostringstream err;
    AssemblyResult r = assemble({"nop", "bogus r1", "add r1, r2", "nop"}, err);
    CHECK(!r.ok());
    CHECK_EQ(r.error_count, 2);
    CHECK_EQ(r.instructions.size(), 2u);   // the two good lines still assembled
    CHECK(err.str().find("Line 2") != std::string::npos);
    CHECK(err.str().find("Line 3") != std::string::npos);
}

TEST(assemble_rejects_out_of_range_values) {
    std::ostringstream err;
    CHECK(!assemble({"add r1, r2, 0x7FFFFFFFFFFF"}, err).ok());   // too big for 41 bits
    CHECK(!assemble({"mov r40, 1"}, err).ok());                   // no such register
}

TEST(assemble_then_disassemble_round_trip) {
    std::ostringstream err;
    const std::vector<std::pair<std::string, std::string>> cases = {
        {"add r3, r1, r2",  "add r3, r1, r2"},
        {"add r4, r3, 1",   "add r4, r3, #1"},
        {"incr r4",         "incr r4"},
        {"cmp r4, 7",       "cmp r4, #7"},
        {"cmp r4, r2",      "cmp r4, r2"},
        {"mov r8, r9",      "mov r8, r9"},
        {"mov r1, -5",      "mov r1, #-5"},
        {"not r3",          "not r3, r3"},
        {"not r3, r4",      "not r3, r4"},     // not rd, rs1
        {"tog r12, r1, 3",  "tog r12, r1, #3"},
        {"st r5, 4[r30]",   "st r5, 4[r30]"},
        {"vld r20, [r2 + #8]", "vld r20, r2, #8"},
        {"ld r5, 0[sp]",    "ld r5, 0[r30]"},
        {"ld r5, -4[r6]",   "ld r5, -4[r6]"},
        {"b #16",           "b #16"},
        {"nop",             "nop"},
    };
    for (const auto& [source, expected] : cases) {
        AssemblyResult r = assemble({source}, err);
        CHECK(r.ok());
        if (r.ok()) CHECK_EQ(disassemble_one(r.instructions[0].word), expected);
    }
}

TEST(assemble_is_case_insensitive_for_mnemonics) {
    std::ostringstream err;
    AssemblyResult a = assemble({"ADD r1, r2, r3"}, err);
    AssemblyResult b = assemble({"add r1, r2, r3"}, err);
    CHECK(a.ok() && b.ok());
    CHECK_EQ(a.instructions[0].word, b.instructions[0].word);
}

TEST(assemble_file_missing_file_throws) {
    std::ostringstream err;
    bool threw = false;
    try { assemble_file("/definitely/not/here.asm", err); } catch (const std::runtime_error&) { threw = true; }
    CHECK(threw);
}

TEST(hex_file_round_trip) {
    const char* path = "risc201_test_tmp.hex";
    const std::vector<uint64_t> words = {0x0, 0xF800000000000000ULL, 0x123456789ABCDEF0ULL};
    risc201::write_hex_file(path, words);
    CHECK(read_hex_file(path) == words);

    // Format: 16 lowercase hex digits per line.
    std::ifstream in(path);
    std::string line;
    std::getline(in, line);
    CHECK_EQ(line, std::string("0000000000000000"));
    std::getline(in, line);
    CHECK_EQ(line, std::string("f800000000000000"));
    std::remove(path);
}

TEST(hex_file_errors) {
    const char* path = "risc201_test_tmp_bad.hex";
    { std::ofstream out(path); out << "0000000000000000\nnot-hex\n"; }
    bool threw = false;
    std::string msg;
    try { read_hex_file(path); } catch (const std::runtime_error& e) { threw = true; msg = e.what(); }
    CHECK(threw);
    CHECK(msg.find(":2:") != std::string::npos);   // names the bad line
    std::remove(path);

    threw = false;
    try { read_hex_file("/definitely/not/here.hex"); } catch (const std::runtime_error&) { threw = true; }
    CHECK(threw);
}

TEST(assembler_rejects_bad_operands) {
    std::ostringstream err;
    for (const char* src : {"tog r1, r2, 32", "tog r1, r2, -1", "not r1, r2, r3", "nop r1",
                            "clr r1", "ld r1, 4[r2", "vld r1, 4", "pushm", "popm 5"}) {
        CHECK(!assemble({src}, err).ok());
    }
}


// End-to-end tests on the programs shipped in examples/.
// RISC201_SOURCE_DIR is defined by the build system.




static std::string example(const std::string& name) {
    return std::string(RISC201_SOURCE_DIR) + "/programs/examples/" + name;
}

TEST(example_factorial_computes_120) {
    std::ostringstream err;
    AssemblyResult r = assemble_file(example("factorial5_loop.asm"), err);
    CHECK(r.ok());
    CHECK_EQ(r.instructions.size(), 8u);

    CPU cpu;
    cpu.load_program(r.words());
    // The program ends with "push r1": the answer is pushed onto the stack.
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 1), 120);
    CHECK_EQ(reg(cpu, 2), 0);
    CHECK_EQ(reg(cpu, 30), 65532);                    // the stack grows down from 65536
    CHECK_EQ(cpu.dmem().readWord(65532), 120u);
}


TEST(stack_macro_expands_multi_register_push_and_pop) {
    std::ostringstream err;
    AssemblyResult r = assemble({"start: push r1, r2, r3", "pop r4,r5", "b start"}, err);
    CHECK(r.ok());
    CHECK_EQ(r.instructions.size(), 6u);                // 3 + 2 + 1
    CHECK_EQ(r.instructions[0].address, 0u);
    CHECK_EQ(r.instructions[2].address, 16u);
    // the branch offset accounts for the extra instructions: from 40 back to 0
    AssemblyResult single = assemble({"start: push r1", "push r2", "push r3", "pop r4", "pop r5", "b start"}, err);
    CHECK(single.ok());
    CHECK(r.words() == single.words());
    // an error on a macro line still points at that line
    AssemblyResult bad = assemble({"nop", "push r1, r99"}, err);
    CHECK(!bad.ok());
}

TEST(pushm_and_popm_expand_to_push_and_pop_with_pop_in_reverse_order) {
    std::ostringstream err;
    AssemblyResult r = assemble({"start: pushm r1, r2, r3", "popm r1, r2, r3", "push r4, r5"}, err);
    CHECK(r.ok());
    CHECK_EQ(r.instructions.size(), 8u);
    const char* want[] = {"push r1", "push r2", "push r3", "pop r3", "pop r2", "pop r1", "push r4", "push r5"};
    for (size_t i = 0; i < 8 && i < r.instructions.size(); ++i)
        CHECK_EQ(disassemble_one(r.instructions[i].word), std::string(want[i]));
}
