#include <cmath>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include "Assembler/risc201_common.h"
#include "datapath/instruction_memory.h"
#include "datapath/register_file.h"
#include "Simulator/cpu.h"
#include "Simulator/decoder.h"
#include "Simulator/simulator.h"
#include "test_framework.h"
#include "test_helpers.h"

using namespace risc201;



TEST(cpu_arithmetic) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 5", "mov r2, 10", "add r3, r1, r2", "add r4, r3, 1",
                     "sub r5, r2, r1", "mul r6, r1, r2", "mov r7, r2", "nop"}));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 3), 15);
    CHECK_EQ(reg(cpu, 4), 16);
    CHECK_EQ(reg(cpu, 5), 5);
    CHECK_EQ(reg(cpu, 6), 50);
    CHECK_EQ(reg(cpu, 7), 10);
}

TEST(cpu_logic_shifts_and_bit_ops) {
    CPU cpu;
    CHECK(boot(cpu, {
        "mov r1, 12", "mov r2, 10",
        "and r3, r1, r2",      // 8
        "or r4, r1, r2",       // 14
        "xor r5, r1, r2",      // 6
        "lsl r6, r1, 2",       // 48
        "lsr r7, r1, 2",       // 3
        "mov r8, -16",
        "asr r9, r8, 2",       // -4
        "mov r10, r1",
        "not r10",            // -13
        "pcnt r11, r1",        // 2
        "tog r12, r1, 0",      // 13
        "sbit r13, r1, 1",     // 14
        "cbit r14, r1, 2",     // 8
        "rotl r15, r1, 1",     // 24
        "mov r16, -7",
        "abs r16",             // 7
        "incr r16",            // 8
        "decr r16",            // 7
    }));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 3), 8);
    CHECK_EQ(reg(cpu, 4), 14);
    CHECK_EQ(reg(cpu, 5), 6);
    CHECK_EQ(reg(cpu, 6), 48);
    CHECK_EQ(reg(cpu, 7), 3);
    CHECK_EQ(reg(cpu, 9), -4);
    CHECK_EQ(reg(cpu, 10), -13);
    CHECK_EQ(reg(cpu, 11), 2);
    CHECK_EQ(reg(cpu, 12), 13);
    CHECK_EQ(reg(cpu, 13), 14);
    CHECK_EQ(reg(cpu, 14), 8);
    CHECK_EQ(reg(cpu, 15), 24);
    CHECK_EQ(reg(cpu, 16), 7);
}

TEST(cpu_beq_taken_and_not_taken) {
    CPU taken;
    CHECK(boot(taken, {"mov r1, 5", "mov r2, 5", "cmp r1, r2", "beq equal",
                       "mov r3, 111", "b done", "equal: mov r3, 222", "done: nop"}));
    CHECK(taken.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(taken, 3), 222);
    CHECK(taken.regs().flag_e());

    CPU not_taken;
    CHECK(boot(not_taken, {"mov r1, 5", "cmp r1, 6", "beq equal",
                           "mov r3, 111", "b done", "equal: mov r3, 222", "done: nop"}));
    CHECK(not_taken.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(not_taken, 3), 111);
}

TEST(cpu_bgt_and_bst) {
    CPU gt;
    CHECK(boot(gt, {"mov r1, 5", "cmp r1, 3", "bgt g", "mov r4, 0", "b end", "g: mov r4, 1", "end: nop"}));
    CHECK(gt.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(gt, 4), 1);

    CPU st;
    CHECK(boot(st, {"mov r1, 5", "cmp r1, 9", "bst s", "mov r5, 0", "b end2", "s: mov r5, 1", "end2: nop"}));
    CHECK(st.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(st, 5), 1);
}

TEST(cpu_factorial_loop) {
    // factorial5_loop.asm without the final push (push semantics undecided).
    const std::vector<std::string> prog = {
        "mov r1, 1", "mov r2, 5",
        "loop:",
        "mul r1, r1, r2", "decr r2", "cmp r2, 0", "bgt loop",
        "nop",
    };
    CPU cpu;
    CHECK(boot(cpu, prog));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 1), 120);
    CHECK_EQ(reg(cpu, 2), 0);
}

TEST(cpu_push_pop_peek_stack_grows_down_from_the_top) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 7", "mov r2, 9", "push r1", "push r2", "peek r3",
                     "pop r4", "pop r5", "nop"}));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 3), 9);
    CHECK_EQ(reg(cpu, 4), 9);
    CHECK_EQ(reg(cpu, 5), 7);
    CHECK_EQ(reg(cpu, 30), 65536);                    // back to an empty stack
    CHECK_EQ(cpu.dmem().readWord(65532), 7u);         // first push went just below the top
    CHECK_EQ(cpu.dmem().readWord(65528), 9u);
}

TEST(cpu_push_writes_big_endian_and_moves_sp_down_by_4) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 0x01020304", "push r1", "nop"}));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 30), 65532);
    CHECK_EQ(cpu.dmem().readByte(65532), 1);
    CHECK_EQ(cpu.dmem().readByte(65535), 4);
}

TEST(cpu_stack_underflow_and_overflow_are_errors) {
    CPU a;
    CHECK(boot(a, {"pop r1"}));
    CHECK(a.run() == StopReason::Error);
    CHECK(a.message().find("underflow") != std::string::npos);

    CPU b;
    CHECK(boot(b, {"peek r1"}));
    CHECK(b.run() == StopReason::Error);

    CPU c;   // 16384 pushes fill the 64 KB memory, the next one overflows
    CHECK(boot(c, {"mov r1, 1", "mov r2, 16385", "loop:", "push r1", "decr r2", "cmp r2, 0", "bgt loop", "nop"}));
    CHECK(c.run(1000000) == StopReason::Error);
    CHECK(c.message().find("overflow") != std::string::npos);
    CHECK_EQ(reg(c, 30), 0);
}

TEST(cpu_mpop_and_clr) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 1", "push r1", "push r1", "push r1", "mpop 2", "nop"}));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 30), 65532);
    CPU d;
    CHECK(boot(d, {"mov r1, 1", "push r1", "push r1", "clr", "nop"}));
    CHECK(d.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(d, 30), 65536);
    CPU e;
    CHECK(boot(e, {"mov r1, 1", "push r1", "mpop 2"}));
    CHECK(e.run() == StopReason::Error);
}

TEST(cpu_ld_st_use_data_memory) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 0x2000", "mov r2, 1234", "st r2, 8[r1]", "ld r3, 8[r1]", "nop"}));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 3), 1234);
    CHECK_EQ(cpu.dmem().readWord(0x2008), 1234u);
}

TEST(cpu_misaligned_and_out_of_range_access_are_errors) {
    CPU a;
    CHECK(boot(a, {"mov r1, 0x2002", "ld r2, 0[r1]"}));
    CHECK(a.run() == StopReason::Error);
    CHECK(a.message().find("Misaligned") != std::string::npos);
    CPU b;
    CHECK(boot(b, {"mov r1, 0x10000", "ld r2, 0[r1]"}));
    CHECK(b.run() == StopReason::Error);
}

TEST(cpu_call_and_ret_including_nested_calls) {
    // call pushes the caller's ra and the return address; ret pops both, so
    // nested calls work without saving ra by hand.
    CPU cpu;
    CHECK(boot(cpu, {
        "call f",              // 0
        "mov r9, 1",           // 8   runs after f returns
        "b done",              // 16
        "f: call g",           // 24
        "mov r8, 2",           // 32
        "ret",                 // 40
        "g: mov r7, 3",        // 48
        "ret",                 // 56
        "done: nop"}));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 7), 3);
    CHECK_EQ(reg(cpu, 8), 2);
    CHECK_EQ(reg(cpu, 9), 1);
    CHECK_EQ(reg(cpu, 30), 65536);   // stack balanced
    CHECK_EQ(reg(cpu, 31), 0);       // ra restored to its value before the first call
}

TEST(cpu_ret_on_empty_stack_is_an_error) {
    CPU cpu;
    CHECK(boot(cpu, {"ret"}));
    CHECK(cpu.run() == StopReason::Error);
    CHECK(cpu.message().find("underflow") != std::string::npos);
}

static float as_float(int32_t bits) {
    float f; static_assert(sizeof f == 4, ""); std::memcpy(&f, &bits, 4); return f;
}

TEST(cpu_floating_point_arithmetic_and_compare) {
    CPU cpu;
    CHECK(boot(cpu, {
        "mov r1, 0x40200000",   // 2.5
        "mov r2, 0x40900000",   // 4.5
        "fadd r3, r1, r2",      // 7.0
        "fsub r4, r2, r1",      // 2.0
        "fmul r5, r1, r2",      // 11.25
        "fdiv r6, r2, r1",      // 1.8
        "fcmp r1, r2",          // 2.5 < 4.5 -> ST
        "mov r7, 0x41100000",   // 9.0
        "sqrt r8, r7",          // 3.0
        "mov r9, 0xC0200000",   // -2.5
        "fabs r10, r9",         // 2.5
        "nop"}));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK(as_float(reg(cpu, 3)) == 7.0f);
    CHECK(as_float(reg(cpu, 4)) == 2.0f);
    CHECK(as_float(reg(cpu, 5)) == 11.25f);
    CHECK(std::fabs(as_float(reg(cpu, 6)) - 1.8f) < 1e-5f);
    CHECK(cpu.regs().flag_st() && !cpu.regs().flag_e() && !cpu.regs().flag_gt());
    CHECK(std::fabs(as_float(reg(cpu, 8)) - 3.0f) < 1e-4f);
    CHECK(as_float(reg(cpu, 10)) == 2.5f);
}

TEST(cpu_end_of_program_is_a_normal_halt) {
    CPU cpu;
    CHECK(boot(cpu, {"nop"}));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(cpu.steps(), 1u);
    CHECK(cpu.halted());
}

TEST(cpu_unknown_opcode_is_an_error) {
    CPU cpu;
    cpu.load_program({0xFE00000000000000ULL});
    CHECK(cpu.run() == StopReason::Error);
    CHECK(cpu.message().find("Unknown opcode") != std::string::npos);
    CHECK_EQ(cpu.regs().ip(), 0);   // ip stays on the faulting instruction
}

TEST(cpu_misaligned_ip_is_an_error) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r29, 4", "nop"}));
    CHECK(cpu.run() == StopReason::Error);
    CHECK(cpu.message().find("Misaligned") != std::string::npos);
}

TEST(cpu_writing_r29_acts_as_a_jump) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r29, 16", "mov r1, 1", "mov r2, 2", "nop"}));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 1), 0);   // skipped
    CHECK_EQ(reg(cpu, 2), 2);
}

TEST(cpu_step_limit_can_be_resumed) {
    CPU cpu;
    CHECK(boot(cpu, {"loop: b loop"}));
    CHECK(cpu.run(100) == StopReason::MaxSteps);
    CHECK_EQ(cpu.steps(), 100u);
    CHECK(!cpu.halted());
    CHECK(cpu.run(50) == StopReason::MaxSteps);
    CHECK_EQ(cpu.steps(), 150u);
}

TEST(cpu_breakpoint_stops_then_resumes_past_it) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 1", "mov r2, 2", "mov r3, 3", "nop"}));
    cpu.add_breakpoint(16);
    CHECK(cpu.run() == StopReason::Breakpoint);
    CHECK_EQ(cpu.regs().ip(), 16);
    CHECK_EQ(reg(cpu, 3), 0);
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 3), 3);
}

TEST(cpu_reset_allows_rerun) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 9", "nop"}));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    cpu.reset();
    CHECK(cpu.regs().ip() == 0 && reg(cpu, 1) == 0 && !cpu.halted() && cpu.steps() == 0);
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 1), 9);
}

TEST(cpu_trace_reports_changed_registers) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 5", "nop"}));
    std::ostringstream trace;
    cpu.set_trace(&trace);
    cpu.run();
    CHECK(trace.str().find("mov") != std::string::npos);
    CHECK(trace.str().find("r1: 0 -> 5") != std::string::npos);
}





TEST(cli_scripted_session) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 5", "mov r2, 10", "add r3, r1, r2", "nop"}));
    std::istringstream in("step 2\nregs\nlist 2\nbreak 0x10\nbreaks\nrun\nreg 3\nquit\n");
    std::ostringstream out;
    run_cli(cpu, in, out);
    const std::string s = out.str();
    CHECK(s.find("Breakpoint set") != std::string::npos);
    CHECK(s.find("Program finished") != std::string::npos);
    CHECK(s.find("r3 = 15") != std::string::npos);
    CHECK_EQ(reg(cpu, 3), 15);
}

TEST(cli_rejects_bad_input_without_crashing) {
    CPU cpu;
    CHECK(boot(cpu, {"nop"}));
    std::istringstream in("frobnicate\nbreak 5\nreg 99\nstep abc\nquit\n");
    std::ostringstream out;
    run_cli(cpu, in, out);
    const std::string s = out.str();
    CHECK(s.find("Unknown command") != std::string::npos);
    CHECK(s.find("multiple of 8") != std::string::npos);
    CHECK(s.find("usage: reg") != std::string::npos);
    CHECK(s.find("usage: step") != std::string::npos);
}

TEST(cli_load_missing_file_reports_error) {
    CPU cpu;
    std::ostringstream err;
    CHECK(!load_program_file(cpu, "/definitely/not/here.asm", err));
    CHECK(!load_program_file(cpu, "/definitely/not/here.hex", err));
    CHECK(err.str().find("Error") != std::string::npos);
}




TEST(decode_r_immediate_and_register_forms) {
    DecodedInstr d = decode(encode_R_IMM(0x00, 3, 1, 7));
    CHECK(d.valid);
    CHECK_EQ(d.mnemonic, std::string("add"));
    CHECK_EQ(d.rd, 3);
    CHECK_EQ(d.rs1, 1);
    CHECK(d.use_imm);
    CHECK_EQ(d.imm, 7);

    d = decode(encode_R(0x00, 4, 1, 2));
    CHECK(!d.use_imm);
    CHECK_EQ(d.rd, 4);
    CHECK_EQ(d.rs2, 2);
}

TEST(decode_negative_immediate_is_sign_extended) {
    DecodedInstr d = decode(encode_R_IMM(0x00, 1, 1, -9));
    CHECK_EQ(d.imm, -9);
    d = decode(encode_BRANCH(0x34, -24));
    CHECK_EQ(d.mnemonic, std::string("bgt"));
    CHECK_EQ(d.imm, -24);
}

TEST(decode_cmp_forms) {
    DecodedInstr d = decode(encode_CMP_IMM(0x05, 2, 0));
    CHECK(d.use_imm);
    CHECK_EQ(d.rs1, 2);
    CHECK_EQ(d.imm, 0);
    d = decode(encode_CMP(0x05, 2, 3));
    CHECK(!d.use_imm);
    CHECK_EQ(d.rs2, 3);
}

TEST(decode_single_register_in_place) {
    DecodedInstr d = decode(encode_R1S(0x06, 5));
    CHECK_EQ(d.mnemonic, std::string("incr"));
    CHECK_EQ(d.rs1, 5);
    CHECK_EQ(d.rd, 0);
}

TEST(decode_bit_ops) {
    DecodedInstr d = decode(encode_BIT(0x21, 12, 1, 9));
    CHECK_EQ(d.mnemonic, std::string("tog"));
    CHECK_EQ(d.rd, 12);
    CHECK_EQ(d.rs1, 1);
    CHECK_EQ(d.bit_idx, 9);
}

TEST(decode_mov_forms) {
    DecodedInstr d = decode(encode_MOVE_IMM(0x42, 1, 0, -5));
    CHECK(d.use_imm);
    CHECK_EQ(d.imm, -5);
    CHECK_EQ(d.mode, 0);
    d = decode(encode_MOVE_REG(0x42, 8, 9, 0));
    CHECK(!d.use_imm);
    CHECK_EQ(d.rd, 8);
    CHECK_EQ(d.rs2, 9);
    d = decode(encode_MOVE_IMM(0x42, 1, 2, 5));
    CHECK_EQ(d.mode, 2);
}

TEST(decode_store_uses_base_in_rd_and_value_in_rs1) {
    DecodedInstr d = decode(encode_MEM_STORE(0x41, /*rd_base=*/30, /*rs1_value=*/5, 4));
    CHECK_EQ(d.mnemonic, std::string("st"));
    CHECK_EQ(d.rd, 30);
    CHECK_EQ(d.rs1, 5);
    CHECK_EQ(d.imm, 4);
}

TEST(decode_unknown_opcode_is_invalid) {
    CHECK(!decode(encode_NO_OPERAND(0x7F)).valid);
}

TEST(instruction_memory_fetch_rules) {
    InstructionMemory m;
    m.load_words({0x11, 0x22});
    CHECK_EQ(m.size_words(), 2u);
    CHECK(m.in_range(0) && m.in_range(8));
    CHECK(!m.in_range(16));
    CHECK(!m.in_range(4));
    CHECK_EQ(m.fetch(8), 0x22u);

    bool misaligned = false, past_end = false;
    try { m.fetch(4); } catch (const std::runtime_error&) { misaligned = true; }
    try { m.fetch(16); } catch (const std::out_of_range&) { past_end = true; }
    CHECK(misaligned);
    CHECK(past_end);
}





TEST(register_file_read_write) {
    RegisterFile rf;
    rf.write(1, 120);
    CHECK_EQ(rf.read(1), 120);
    rf.write(2, -5);
    CHECK_EQ(rf.read(2), -5);
}

TEST(register_file_special_registers) {
    RegisterFile rf;
    rf.set_ip(8);
    rf.set_sp(0x1000);
    rf.set_ra(24);
    CHECK_EQ(rf.read(29), 8);
    CHECK_EQ(rf.read(30), 0x1000);
    CHECK_EQ(rf.read(31), 24);
    CHECK_EQ(rf.ip(), 8);
    CHECK_EQ(rf.sp(), 0x1000);
    CHECK_EQ(rf.ra(), 24);
}

TEST(register_file_flags) {
    RegisterFile rf;
    CHECK(!rf.flag_e() && !rf.flag_gt() && !rf.flag_st());
    rf.set_flags(false, true, false);
    CHECK(rf.flag_gt() && !rf.flag_e() && !rf.flag_st());
}

TEST(register_file_bounds_and_reset) {
    RegisterFile rf;
    bool threw = false;
    try { rf.read(32); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { rf.write(40, 1); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);

    rf.write(1, 7);
    rf.set_flags(true, false, false);
    rf.reset();
    CHECK_EQ(rf.read(1), 0);
    CHECK(!rf.flag_e());
}

TEST(register_file_dump_mentions_special_registers) {
    RegisterFile rf;
    const std::string s = rf.dump();
    CHECK(s.find("r29(ip)") != std::string::npos);
    CHECK(s.find("r30(sp)") != std::string::npos);
    CHECK(s.find("flags:") != std::string::npos);
}


TEST(debugger_stack_command_draws_the_stack) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 42", "push r1", "nop"}));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    std::ostringstream o;
    show_stack(cpu, o);
    CHECK(o.str().find("1 words used") != std::string::npos);
    CHECK(o.str().find("0x0000002a") != std::string::npos);
    CHECK(o.str().find("top") != std::string::npos);
}

TEST(debugger_stage_command_walks_if_id_ex_wb_then_commits) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 5", "add r2, r1, 1", "nop"}));
    std::istringstream in("stage\nstage\nstage\n");
    std::ostringstream o;
    run_cli(cpu, in, o);
    CHECK(o.str().find("[IF]") != std::string::npos);
    CHECK(o.str().find("[OF]") != std::string::npos);
    CHECK(o.str().find("[EX] execute: result r1: 0 -> 5") != std::string::npos);
    CHECK_EQ(reg(cpu, 1), 0);                   // not committed until WB
    CPU cpu2;
    CHECK(boot(cpu2, {"mov r1, 5", "add r2, r1, 1", "nop"}));
    std::istringstream in2("stage\nstage\nstage\nstage\n");
    std::ostringstream o2;
    run_cli(cpu2, in2, o2);
    CHECK_EQ(reg(cpu2, 1), 5);                  // WB committed it
    CHECK_EQ(cpu2.regs().ip(), 8);
}

TEST(debugger_micro_command_previews_microcode_without_executing) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 5", "add r2, r1, 1", "nop"}));
    cpu.step();
    std::istringstream in("micro\n");
    std::ostringstream o;
    run_cli(cpu, in, o);
    CHECK(o.str().find("MLOAD_IR") != std::string::npos);
    CHECK(o.str().find("MDECODE") != std::string::npos);
    CHECK(o.str().find("r2: 0 -> 6") != std::string::npos);   // result of the preview
    CHECK_EQ(reg(cpu, 2), 0);                                  // real CPU untouched
    CHECK_EQ(cpu.regs().ip(), 8);
}

TEST(debugger_six_stage_view_splits_memory_access_and_writeback) {
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 7", "push r1", "nop"}));
    cpu.step();
    std::istringstream in("pipeline 6\nstage\nstage\nstage\nstage\nstage\n");
    std::ostringstream o;
    run_cli(cpu, in, o);
    const std::string out = o.str();
    for (const char* n : {"[IF]", "[ID]", "[RR]", "[EX]", "[MA]"})
        CHECK(out.find(n) != std::string::npos);
    CHECK(out.find("[MA] memory access: mem[0xfffc]: 0 -> 7") != std::string::npos);
    CHECK_EQ(reg(cpu, 30), 65536);                  // not committed yet: RW has not run
    std::istringstream in2("pipeline 6\nstage\nstage\nstage\nstage\nstage\nstage\n");
    std::ostringstream o2;
    CPU cpu2;
    CHECK(boot(cpu2, {"mov r1, 7", "push r1", "nop"}));
    cpu2.step();
    run_cli(cpu2, in2, o2);
    CHECK(o2.str().find("[RW] write-back: r30: 65536 -> 65532") != std::string::npos);
    CHECK_EQ(reg(cpu2, 30), 65532);
}

// Runs the same program hardwired and through the microprogrammed control unit
// and requires identical registers, flags and data memory.
static void expect_same_in_both_modes(const std::vector<std::string>& prog, uint64_t* micro_count = nullptr) {
    CPU a, b;
    CHECK(boot(a, prog));
    CHECK(boot(b, prog));
    b.set_micro_mode(true);
    a.run();
    b.run();
    CHECK(a.last_stop() == b.last_stop());
    for (int i = 0; i < 32; ++i) CHECK_EQ(reg(a, i), reg(b, i));
    CHECK(a.regs().flag_e() == b.regs().flag_e());
    CHECK(a.regs().flag_gt() == b.regs().flag_gt());
    CHECK(a.regs().flag_st() == b.regs().flag_st());
    bool same_memory = true;
    for (uint32_t addr = 0; addr < CPU::kDataMemorySize; addr += 4)
        if (a.dmem().readWord(addr) != b.dmem().readWord(addr)) same_memory = false;
    CHECK(same_memory);
    if (micro_count) *micro_count = b.micro_instructions();
}

TEST(micro_mode_matches_hardwired_arithmetic_logic_and_branches) {
    uint64_t n = 0;
    expect_same_in_both_modes({"mov r1, 12", "mov r2, 10", "add r3, r1, r2", "sub r4, r1, r2", "mul r5, r1, r2",
        "and r6, r1, r2", "or r7, r1, r2", "xor r8, r1, r2", "lsl r9, r1, 2", "lsr r10, r1, 1", "not r11",
        "cmp r1, r2", "bgt skip", "mov r12, 99", "skip: incr r2", "decr r1", "nop"}, &n);
    CHECK(n > 10);   // most instructions really went through microcode
}

TEST(micro_mode_matches_hardwired_loop_and_memory) {
    uint64_t n = 0;
    expect_same_in_both_modes({"mov r1, 1", "mov r2, 5", "loop:", "mul r1, r1, r2", "decr r2", "cmp r2, 0", "bgt loop",
        "mov r3, 0x2000", "st r1, 4[r3]", "ld r4, 4[r3]", "push r4", "pop r5"}, &n);
    CHECK(n > 10);
}

TEST(micro_mode_matches_hardwired_floating_point) {
    expect_same_in_both_modes({"mov r1, 0x40200000", "mov r2, 0x40900000", "fadd r3, r1, r2", "fmul r4, r1, r2",
        "fcmp r1, r2", "mov r5, 0x41100000", "sqrt r6, r5"});
}

TEST(micro_mode_runs_stack_and_calls_through_microcode) {
    // Same program in both modes, including push/pop/call/ret and mov #imm.
    uint64_t n = 0;
    expect_same_in_both_modes({"mov r1, 6", "call f", "b done", "f: push r1", "mpop 0", "pop r2", "call g", "ret", "g: peek r3", "push r2", "peek r4", "pop r5", "ret", "done: nop"}, &n);
    CPU cpu;
    CHECK(boot(cpu, {"mov r1, 6", "call f", "b done", "f: push r1", "pop r2", "ret", "done: nop"}));
    cpu.set_micro_mode(true);
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 2), 6);
    CHECK_EQ(cpu.micro_fallbacks(), 0u);          // every instruction has microcode now
    CHECK_EQ(cpu.micro_instructions(), 7u);
    CHECK_EQ(reg(cpu, 30), 65536);
}

TEST(micro_mode_reports_the_same_stack_errors) {
    CPU a, b;
    CHECK(boot(a, {"pop r1"}));
    CHECK(boot(b, {"pop r1"}));
    b.set_micro_mode(true);
    CHECK(a.run() == StopReason::Error);
    CHECK(b.run() == StopReason::Error);
    CHECK(a.message() == b.message());
}

TEST(vector_ops_in_micro_mode_compute_add_dot_sub_cross) {
    CPU cpu;
    cpu.set_micro_mode(true);
    CHECK(boot(cpu, {"mov r20, 0x3F800000", "mov r21, 0x40000000", "mov r22, 0x40400000",
                     "mov r23, 0x40800000", "mov r24, 0x40A00000", "mov r25, 0x40C00000",
                     "vadd r26, r20, r23", "vdot r5, r20, r23"}));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(static_cast<uint32_t>(reg(cpu, 26)), 0x40A00000u);   // 5.0
    CHECK_EQ(static_cast<uint32_t>(reg(cpu, 27)), 0x40E00000u);   // 7.0
    CHECK_EQ(static_cast<uint32_t>(reg(cpu, 28)), 0x41100000u);   // 9.0
    CHECK_EQ(static_cast<uint32_t>(reg(cpu, 5)),  0x42000000u);   // 32.0
}

TEST(vector_ops_reject_invalid_vector_ids_in_both_modes) {
    for (bool micro : {false, true}) {
        CPU cpu;
        cpu.set_micro_mode(micro);
        CHECK(boot(cpu, {"vadd r20, r23, r27"}));   // r27 is not a vector operand
        CHECK(cpu.run() == StopReason::Error);
        CHECK(cpu.message().find("not a valid vector id") != std::string::npos);

        CPU dot;
        dot.set_micro_mode(micro);
        CHECK(boot(dot, {"vdot r5, r20, r7"}));          // rs2 = r7 is not a vector id
        CHECK(dot.run() == StopReason::Error);
        CHECK(dot.message().find("vs2") != std::string::npos);
    }
}

TEST(vector_load_store_work_without_micro_mode_and_check_the_vector_id) {
    CPU cpu;   // hardwired mode: vector instructions still run through the control unit
    CHECK(boot(cpu, {"mov r20, 7", "mov r21, 8", "mov r22, 9", "mov r4, 0x100",
                     "vst r20, r4, #0",    // store v1 (r20-r22) at 0x100
                     "vld r23, r4, #0"}));  // load it into v2 (r23-r25)
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(reg(cpu, 23), 7);
    CHECK_EQ(reg(cpu, 24), 8);
    CHECK_EQ(reg(cpu, 25), 9);

    CPU bad;
    CHECK(boot(bad, {"mov r4, 0x100", "vst r5, r4, #0"}));    // r5 is not a vector operand
    CHECK(bad.run() == StopReason::Error);
    CHECK(bad.message().find("not a valid vector id") != std::string::npos);
}

TEST(pipeline_model_matches_isa_sim_on_factorial) {
    CPU cpu;
    std::ostringstream err;
    CHECK(load_program_file(cpu, std::string(RISC201_SOURCE_DIR) + "/programs/examples/factorial.asm", err));
    CHECK(cpu.run() == StopReason::EndOfProgram);
    CHECK_EQ(cpu.pipe_trace().size(), static_cast<size_t>(cpu.steps()));

    PipeStats four, six;
    run_pipeline_model(cpu.pipe_trace(), false, four, six);   // values printed by isa_sim
    CHECK_EQ(four.instructions, 24LL);
    CHECK_EQ(four.cycles, 41LL);  CHECK_EQ(six.cycles, 53LL);
    CHECK_EQ(four.stalls, 6LL);   CHECK_EQ(six.stalls, 12LL);
    CHECK_EQ(four.flushes, 4LL);  CHECK_EQ(six.flushed_slots, 12LL);

    run_pipeline_model(cpu.pipe_trace(), true, four, six);    // with forwarding
    CHECK_EQ(four.cycles, 35LL);  CHECK_EQ(six.cycles, 41LL);
    CHECK_EQ(four.stalls, 0LL);

    cpu.reset();
    CHECK(cpu.pipe_trace().empty());                           // reset clears the recorded trace
}
