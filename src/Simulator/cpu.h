#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "ALU/alu/alu.h"
#include "ALU/fpu/fpu.h"
#include "Simulator/decoder.h"
#include "Simulator/pipeline_model.h"
#include "datapath/instruction_memory.h"
#include "datapath/memory.h"
#include "datapath/register_file.h"

namespace risc201 {

class MicroExecutor;

// Why the CPU stopped (or None if the last step simply executed).
enum class StopReason {
    None,            // last step executed normally
    EndOfProgram,    // ip moved past the last instruction (normal halt)
    Error,           // bad opcode, misaligned/invalid ip, bad branch target...
    NotImplemented,  // valid instruction whose semantics are not decided yet
    MaxSteps,        // run() hit its step limit (can be resumed)
    Breakpoint       // run() stopped before a breakpoint (can be resumed)
};

const char* to_string(StopReason r);

// Thrown internally for instructions that decode fine but have no defined
// behaviour yet (vector, mov modes 1/2).
class NotImplementedError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// RISC201 CPU:
//   fetch(ip) -> decode -> run the instruction through the microprogrammed control unit -> update ip
//
// Conventions:
//   * ip (r29) holds the byte address of the instruction being executed.
//     Branch targets are ip + offset (offset relative to the branch itself).
//   * If an instruction explicitly writes r29 (e.g. "mov r29, 16"), that
//     becomes the next ip, i.e. it acts as a jump.
//   * On an error the ip stays at the faulting instruction.
//
// Data memory (separate from instruction memory): 64 KB, big-endian, 4-byte
// words that must be 4-byte aligned. This matches the control unit's memory.
// The stack lives at the TOP of data memory and grows DOWN (same as the team's
// stack microcode): sp starts at 65536 (empty stack) and always points at the
// newest word.
//   push: sp -= 4; mem[sp] = reg      pop: reg = mem[sp]; sp += 4
//   peek: reg = mem[sp]               mpop #n: sp += 4n          clr: sp = 65536
//   call: push ra; push (ip + 8); ra = ip + 8; jump        ret: ip = pop; ra = pop
// Overflow (no room left), underflow (not enough words) and misaligned or
// out-of-range ld/st stop the CPU with an error.
class CPU {
public:
    static constexpr uint32_t kDataMemorySize = 65536;   // bytes
    static constexpr uint32_t kStackTop = 65536;         // sp value of an empty stack

    CPU();

    // Loading a program also resets the CPU. Breakpoints are kept.
    void load_program(const std::vector<uint64_t>& words);
    void load_hex(const std::string& path);   // assembler .hex output

    // Clears registers, flags, ip, step count and halt state.
    void reset();

    // Executes one instruction. Returns StopReason::None if it ran normally,
    // otherwise the reason the CPU halted (it stays halted until reset()).
    StopReason step();

    // Runs until the CPU halts, a breakpoint is reached, or max_steps
    // instructions have executed. A breakpoint at the current ip is ignored
    // for the first instruction so you can resume from it.
    StopReason run(uint64_t max_steps = 1000000);

    bool halted() const { return halted_; }
    StopReason last_stop() const { return stop_; }
    const std::string& message() const { return message_; }   // detail for Error/NotImplemented/EndOfProgram
    uint64_t steps() const { return steps_; }

    // Every executed instruction (dependencies and branch outcome) for the pipeline timing model.
    // Recording stops after kMaxPipeTrace entries; pipe_trace_truncated() then returns true.
    static constexpr size_t kMaxPipeTrace = 1000000;
    const std::vector<TraceEntry>& pipe_trace() const { return pipe_trace_; }
    bool pipe_trace_truncated() const { return pipe_trace_truncated_; }

    RegisterFile& regs() { return regs_; }
    const RegisterFile& regs() const { return regs_; }
    const InstructionMemory& imem() const { return imem_; }
    memory::Memory& dmem() { return dmem_; }
    const memory::Memory& dmem() const { return dmem_; }

    // Breakpoints are byte addresses (multiples of 8).
    void add_breakpoint(uint32_t addr) { breakpoints_.insert(addr); }
    void remove_breakpoint(uint32_t addr) { breakpoints_.erase(addr); }
    void clear_breakpoints() { breakpoints_.clear(); }
    const std::set<uint32_t>& breakpoints() const { return breakpoints_; }

    // Number of instructions executed so far (every one runs through the control unit's microcode).
    uint64_t micro_instructions() const { return micro_count_; }

    // When set, every executed instruction prints one line: address,
    // disassembly, and the registers/flags it changed. Pass nullptr to turn off.
    void set_trace(std::ostream* os) { trace_ = os; }

private:
    // Runs one decoded instruction; returns the next ip.
    uint32_t execute(const DecodedInstr& d, uint32_t pc);
    void halt(StopReason why, const std::string& msg);
    void require_stack_words(uint32_t words, const char* what) const;
    void check_stack_op(const DecodedInstr& d) const;
    void check_vector_op(const DecodedInstr& d) const;
    void check_memory_op(const DecodedInstr& d) const;
    uint32_t data_address(int32_t base, int64_t offset) const;

    RegisterFile regs_;
    InstructionMemory imem_;
    memory::Memory dmem_{kDataMemorySize};
    std::set<uint32_t> breakpoints_;
    std::ostream* trace_ = nullptr;

    std::shared_ptr<MicroExecutor> micro_;   // created on first use; shared by copies
    uint64_t micro_count_ = 0;

    bool halted_ = false;
    StopReason stop_ = StopReason::None;
    std::string message_;
    uint64_t steps_ = 0;
    std::vector<TraceEntry> pipe_trace_;
    bool pipe_trace_truncated_ = false;
};

}  // namespace risc201
