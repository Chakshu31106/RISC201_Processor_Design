#include "Simulator/cpu.h"

#include <iomanip>
#include <sstream>

#include "Assembler/risc201_common.h"
#include "Simulator/micro_executor.h"

namespace risc201 {

const char* to_string(StopReason r) {
    switch (r) {
        case StopReason::None:           return "running";
        case StopReason::EndOfProgram:   return "end of program";
        case StopReason::Error:          return "error";
        case StopReason::NotImplemented: return "not implemented";
        case StopReason::MaxSteps:       return "step limit reached";
        case StopReason::Breakpoint:     return "breakpoint";
    }
    return "?";
}

namespace {

std::string hex_addr(uint32_t a) {
    std::ostringstream o;
    o << "0x" << std::hex << std::setw(4) << std::setfill('0') << a;
    return o.str();
}

[[noreturn]] void not_implemented(const std::string& what) {
    throw NotImplementedError("'" + what + "' is not implemented yet");
}

} // namespace

CPU::CPU() { reset(); }

void CPU::load_program(const std::vector<uint64_t>& words) {
    imem_.load_words(words);
    reset();
}

void CPU::load_hex(const std::string& path) {
    imem_.load_hex(path);
    reset();
}

void CPU::reset() {
    regs_.reset();
    regs_.write(RegisterFile::SP, static_cast<int32_t>(kStackTop));   // empty stack
    dmem_ = memory::Memory(kDataMemorySize);
    halted_ = false;
    stop_ = StopReason::None;
    message_.clear();
    steps_ = 0;
    pipe_trace_.clear();
    pipe_trace_truncated_ = false;
    micro_count_ = 0;
}

// Stack underflow: fewer than `words` words are on the stack.
void CPU::require_stack_words(uint32_t words, const char* what) const {
    const uint64_t sp = static_cast<uint32_t>(regs_.sp());
    if (sp + 4ull * words > kStackTop)
        throw std::runtime_error(std::string("Stack underflow (") + what + ")");
}

// Explicit stack overflow/underflow errors, checked before the control unit runs
// push / pop / peek / mpop / call / ret (the microcode itself would only fail with
// a bare "memory address out of range").
void CPU::check_stack_op(const DecodedInstr& d) const {
    const std::string& m = d.mnemonic;
    const uint32_t sp = static_cast<uint32_t>(regs_.sp());
    if (m == "push") { if (sp < 4 || sp > kStackTop) throw std::runtime_error("Stack overflow (data memory is full)"); }
    else if (m == "call") { if (sp < 8 || sp > kStackTop) throw std::runtime_error("Stack overflow (data memory is full)"); }
    else if (m == "pop" || m == "peek") require_stack_words(1, "stack is empty");
    else if (m == "ret") require_stack_words(2, "ret needs a return address and the saved ra");
    else if (m == "mpop") {
        if (d.imm < 0 || d.imm > kStackTop / 4) throw std::runtime_error("Stack underflow (mpop count too large)");
        require_stack_words(static_cast<uint32_t>(d.imm), "mpop count too large");
    }
}

// Vector operands are vector ids, not register numbers: 0 = v1 (r20-r22), 1 = v2 (r23-r25),
// 2 = v3 (r26-r28). The microcode silently ends the instruction on any other id, so the
// simulator checks the ids first and reports a clear error instead.
void CPU::check_vector_op(const DecodedInstr& d) const {
    auto vec = [&](uint8_t id, const char* field) {
        if (id > 2)
            throw std::runtime_error(d.mnemonic + ": " + field + " = r" + std::to_string(id) +
                " is not a valid vector id; vector operands must be r20 (v1), r23 (v2) or r26 (v3)");
    };
    switch (d.format) {
        case Format::VEC_3REG:                       // vadd vsub vcrs: vd, vs1, vs2
            vec(d.rd, "vd"); vec(d.rs1, "vs1"); vec(d.rs2, "vs2"); break;
        case Format::VEC_SCALAR2:                    // vdot vang: scalar rd, vs1, vs2
            vec(d.rs1, "vs1"); vec(d.rs2, "vs2"); break;
        case Format::VEC_SCALAR1:                    // vmag: scalar rd, vs1
            vec(d.rs1, "vs1"); break;
        case Format::VEC_LOAD:                       // vld: vd, scalar base, offset
            vec(d.rd, "vd"); break;
        case Format::VEC_STORE:                      // vst: field rd = scalar base, field rs1 = vector
            vec(d.rs1, "vs"); break;
        default: break;
    }
}

uint32_t CPU::data_address(int32_t base, int64_t offset) const {
    const int64_t a = static_cast<int64_t>(static_cast<uint32_t>(base)) + offset;
    if (a < 0 || a >= static_cast<int64_t>(dmem_.size()))
        throw std::runtime_error("Data memory address " + hex_addr(static_cast<uint32_t>(a)) + " out of range");
    return static_cast<uint32_t>(a);
}

// Clear out-of-range errors for ld / st / vld / vst, checked before the control unit runs them
// (the microcode itself would only report a bare "memory address out of range").
void CPU::check_memory_op(const DecodedInstr& d) const {
    const std::string& m = d.mnemonic;
    const int32_t base = (m == "st" || m == "vst") ? regs_.read(d.rd) : regs_.read(d.rs1);
    if (m == "ld" || m == "st") data_address(base, d.imm);
    else if (m == "vld" || m == "vst") { data_address(base, d.imm); data_address(base, d.imm + 8); }
}

void CPU::halt(StopReason why, const std::string& msg) {
    halted_ = true;
    stop_ = why;
    message_ = msg;
}

StopReason CPU::step() {
    if (halted_) return stop_;

    stop_ = StopReason::None;
    message_.clear();

    const uint32_t pc = static_cast<uint32_t>(regs_.ip());

    try {
        // Normal halt: ip is aligned but past the last instruction.
        if (pc % 8 == 0 && !imem_.in_range(pc)) {
            halt(StopReason::EndOfProgram,
                 "ip " + hex_addr(pc) + " is past the end of the program");
            return stop_;
        }

        const uint64_t word = imem_.fetch(pc);   // throws if misaligned
        const DecodedInstr d = decode(word);
        if (!d.valid) {
            std::ostringstream o;
            o << "Unknown opcode 0x" << std::hex << static_cast<int>(d.opcode);
            throw std::runtime_error(o.str());
        }

        // Snapshot for the trace (only when tracing).
        int32_t before[RegisterFile::NUM_REGS] = {};
        bool fe = false, fg = false, fs = false;
        if (trace_) {
            for (int i = 0; i < RegisterFile::NUM_REGS; ++i) before[i] = regs_.read(static_cast<uint8_t>(i));
            fe = regs_.flag_e(); fg = regs_.flag_gt(); fs = regs_.flag_st();
        }

        uint32_t next = execute(d, pc);

        // An instruction that wrote r29 directly redirects execution.
        if (static_cast<uint32_t>(regs_.ip()) != pc)
            next = static_cast<uint32_t>(regs_.ip());

        regs_.set_ip(static_cast<int32_t>(next));
        ++steps_;
        if (pipe_trace_.size() < kMaxPipeTrace) pipe_trace_.push_back(make_trace_entry(d, next != pc + 8));
        else pipe_trace_truncated_ = true;

        if (trace_) {
            std::ostringstream line;
            line << hex_addr(pc) << "  " << std::left << std::setw(24) << disassemble_one(word) << std::right;
            bool any = false;
            for (int i = 0; i < RegisterFile::NUM_REGS; ++i) {
                if (i == RegisterFile::IP) continue;   // ip changes every step
                int32_t now = regs_.read(static_cast<uint8_t>(i));
                if (now != before[i]) {
                    line << (any ? ", " : "| ") << "r" << i << ": " << value_text(before[i]) << " -> " << value_text(now);
                    any = true;
                }
            }
            if (regs_.flag_e() != fe || regs_.flag_gt() != fg || regs_.flag_st() != fs) {
                line << (any ? ", " : "| ") << "flags E=" << regs_.flag_e()
                     << " GT=" << regs_.flag_gt() << " ST=" << regs_.flag_st();
            }
            *trace_ << line.str() << "\n";
        }
    } catch (const NotImplementedError& e) {
        halt(StopReason::NotImplemented, std::string(e.what()) + " at " + hex_addr(pc));
    } catch (const std::exception& e) {
        halt(StopReason::Error, std::string(e.what()) + " at " + hex_addr(pc));
    }
    return stop_;
}

StopReason CPU::run(uint64_t max_steps) {
    uint64_t executed = 0;
    while (true) {
        if (halted_) return stop_;
        if (executed >= max_steps) {
            stop_ = StopReason::MaxSteps;
            return stop_;
        }
        if (executed > 0 && breakpoints_.count(static_cast<uint32_t>(regs_.ip()))) {
            stop_ = StopReason::Breakpoint;
            return stop_;
        }
        step();
        ++executed;
        if (halted_) return stop_;
    }
}

// Every instruction runs through the microprogrammed control unit. The checks first turn the
// control unit's bare errors into clear messages (invalid vector id, stack overflow/underflow,
// address out of range).
uint32_t CPU::execute(const DecodedInstr& d, uint32_t pc) {
    check_vector_op(d);
    check_stack_op(d);
    check_memory_op(d);
    if (!micro_) micro_ = std::make_shared<MicroExecutor>();
    if (!micro_->can_run(d)) not_implemented(d.mnemonic);
    ++micro_count_;
    return micro_->execute(d, pc, regs_, dmem_);
}

}  // namespace risc201
