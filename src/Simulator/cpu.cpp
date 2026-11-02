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
    alu_ = ALU();
    fpu_ = FPU();
    halted_ = false;
    stop_ = StopReason::None;
    message_.clear();
    steps_ = 0;
    pipe_trace_.clear();
    pipe_trace_truncated_ = false;
    micro_count_ = 0;
    fallback_count_ = 0;
}

void CPU::set_micro_mode(bool on) {
    micro_mode_ = on;
    if (on && !micro_) micro_ = std::make_shared<MicroExecutor>();
}

void CPU::stack_push(int32_t value) {
    const uint32_t sp = static_cast<uint32_t>(regs_.sp());
    if (sp < 4 || sp > kStackTop) throw std::runtime_error("Stack overflow (data memory is full)");
    dmem_.writeWord(sp - 4, static_cast<uint32_t>(value));
    regs_.write(RegisterFile::SP, static_cast<int32_t>(sp - 4));
}

// Stack underflow: fewer than `words` words are on the stack.
void CPU::require_stack_words(uint32_t words, const char* what) const {
    const uint64_t sp = static_cast<uint32_t>(regs_.sp());
    if (sp + 4ull * words > kStackTop)
        throw std::runtime_error(std::string("Stack underflow (") + what + ")");
}

int32_t CPU::stack_peek() const {
    require_stack_words(1, "stack is empty");
    return static_cast<int32_t>(dmem_.readWord(static_cast<uint32_t>(regs_.sp())));
}

int32_t CPU::stack_pop() {
    const int32_t v = stack_peek();
    regs_.write(RegisterFile::SP, regs_.sp() + 4);
    return v;
}

void CPU::stack_drop(int64_t count) {
    if (count < 0 || count > kStackTop / 4) throw std::runtime_error("Stack underflow (mpop count too large)");
    require_stack_words(static_cast<uint32_t>(count), "mpop count too large");
    regs_.write(RegisterFile::SP, regs_.sp() + static_cast<int32_t>(4 * count));
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
                    line << (any ? ", " : "| ") << "r" << i << ": " << before[i] << " -> " << now;
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

uint32_t CPU::execute(const DecodedInstr& d, uint32_t pc) {
    const std::string& m = d.mnemonic;
    uint32_t next = pc + 8;

    check_vector_op(d);   // vector ids must be 0, 1 or 2 (both execution modes)

    // Vector instructions exist only as microcode in the control unit (they call the FPU),
    // so they always run through it, even in hardwired mode.
    const bool vector_op = d.format == Format::VEC_3REG || d.format == Format::VEC_SCALAR1 ||
                           d.format == Format::VEC_SCALAR2 || d.format == Format::VEC_LOAD ||
                           d.format == Format::VEC_STORE;
    if (vector_op && !micro_) micro_ = std::make_shared<MicroExecutor>();

    if (micro_mode_ || vector_op) {
        if (micro_->can_run(d)) {
            check_stack_op(d);   // same explicit stack errors as the hardwired path
            ++micro_count_;
            return micro_->execute(d, pc, regs_, dmem_);
        }
        ++fallback_count_;
    }

    // Second ALU operand for formats with an I-bit: immediate or rs2.
    auto operand2 = [&]() -> int32_t {
        return d.use_imm ? static_cast<int32_t>(d.imm) : regs_.read(d.rs2);
    };

    switch (d.format) {

        case Format::R:
        case Format::R_IMM: {
            const int32_t a = regs_.read(d.rs1);
            const int32_t b = operand2();
            int32_t r = 0;
            if      (m == "add")  r = alu_.add(a, b);
            else if (m == "sub")  r = alu_.sub(a, b);
            else if (m == "mul")  r = alu_.mul(a, b);
            else if (m == "div")  r = alu_.div(a, b);
            else if (m == "mod")  r = alu_.mod(a, b);
            else if (m == "pow")  r = alu_.pow(a, b);
            else if (m == "and")  r = alu_.AND(a, b);
            else if (m == "or")   r = alu_.OR(a, b);
            else if (m == "nor")  r = alu_.NOR(a, b);
            else if (m == "nand") r = alu_.NAND(a, b);
            else if (m == "xor")  r = alu_.XOR(a, b);
            else if (m == "lsl")  r = alu_.lsl(a, b);
            else if (m == "lsr")  r = alu_.lsr(a, b);
            else if (m == "asr")  r = alu_.asr(a, b);
            else if (m == "rotl") r = alu_.rotl(a, b);
            else if (m == "rotr") r = alu_.rotr(a, b);
            else not_implemented(m);
            regs_.write(d.rd, r);
            break;
        }

        case Format::CMP: {
            alu_.cmp(regs_.read(d.rs1), operand2());
            regs_.set_flags(alu_.getE(), alu_.getGT(), alu_.getST());
            break;
        }

        case Format::R1: {
            const int32_t a = regs_.read(d.rs1);
            int32_t r = 0;
            if      (m == "not")  r = alu_.NOT(a);
            else if (m == "pcnt") r = alu_.pcnt(a);
            else not_implemented(m);
            regs_.write(d.rd, r);
            break;
        }

        case Format::R1S: {
            // incr / decr / abs: read rs1, write the result back to rs1.
            const int32_t a = regs_.read(d.rs1);
            int32_t r = 0;
            if      (m == "incr") r = alu_.incr(a);
            else if (m == "decr") r = alu_.decr(a);
            else if (m == "abs")  r = alu_.abs(a);
            else not_implemented(m);
            regs_.write(d.rs1, r);
            break;
        }

        case Format::BIT: {
            const int32_t a = regs_.read(d.rs1);
            int32_t r = 0;
            if      (m == "tog")  r = alu_.tog(a, d.bit_idx);
            else if (m == "sbit") r = alu_.sbit(a, d.bit_idx);
            else if (m == "cbit") r = alu_.cbit(a, d.bit_idx);
            else not_implemented(m);
            regs_.write(d.rd, r);
            break;
        }

        case Format::BRANCH: {
            bool take = false;
            if      (m == "b")   take = true;
            else if (m == "beq") take = regs_.flag_e();
            else if (m == "bgt") take = regs_.flag_gt();
            else if (m == "bst") take = regs_.flag_st();
            else if (m != "call") not_implemented(m);
            if (m == "call") {
                // Save the caller's ra, then this call's return address, on the stack.
                stack_push(regs_.ra());
                stack_push(static_cast<int32_t>(pc + 8));
                regs_.write(RegisterFile::RA, static_cast<int32_t>(pc + 8));
                take = true;
            }
            if (take) {
                // Offset is relative to the branch instruction's OWN address.
                int64_t target = static_cast<int64_t>(pc) + d.imm;
                if (target < 0 || target > 0xFFFFFFFFLL)
                    throw std::runtime_error("Branch target out of range");
                next = static_cast<uint32_t>(target);
            }
            break;
        }

        case Format::MOVE: {
            if (d.mode != 0)
                throw NotImplementedError("mov mode " + std::to_string(d.mode) +
                                          " (unsigned/movh) is not implemented yet");
            // The immediate is up to 39 bits; registers are 32 bits, so it is truncated.
            regs_.write(d.rd, d.use_imm ? static_cast<int32_t>(d.imm) : regs_.read(d.rs2));
            break;
        }

        case Format::BRANCH_NOOP: {      // ret
            require_stack_words(2, "ret needs a return address and the saved ra");
            next = static_cast<uint32_t>(stack_pop());
            regs_.write(RegisterFile::RA, stack_pop());
            break;
        }

        case Format::MEM_LOAD: {         // ld rd, rs1, #off
            regs_.write(d.rd, static_cast<int32_t>(dmem_.readWord(data_address(regs_.read(d.rs1), d.imm))));
            break;
        }

        case Format::MEM_STORE: {        // st rs1(value), rd(base), #off
            dmem_.writeWord(data_address(regs_.read(d.rd), d.imm),
                            static_cast<uint32_t>(regs_.read(d.rs1)));
            break;
        }

        case Format::STACK_1: {
            if      (m == "push") stack_push(regs_.read(d.rd));
            else if (m == "pop")  { const int32_t v = stack_pop(); regs_.write(d.rd, v); }
            else if (m == "peek") regs_.write(d.rd, stack_peek());
            else not_implemented(m);
            break;
        }

        case Format::STACK_N_COUNT:      // mpop #n
            stack_drop(d.imm);
            break;

        case Format::STACK_N_NOOP:       // clr
            regs_.write(RegisterFile::SP, static_cast<int32_t>(kStackTop));
            break;

        // Floating point: registers hold raw IEEE-754 single-precision bit
        // patterns (e.g. "mov r1, 0x40490fdb" loads 3.14159).
        case Format::FP_R: {
            const uint32_t a = static_cast<uint32_t>(regs_.read(d.rs1));
            const uint32_t b = static_cast<uint32_t>(regs_.read(d.rs2));
            uint32_t r = 0;
            if      (m == "fadd") r = fpu_.fadd(a, b);
            else if (m == "fsub") r = fpu_.fsub(a, b);
            else if (m == "fmul") r = fpu_.fmul(a, b);
            else if (m == "fdiv") r = fpu_.fdiv(a, b);
            else not_implemented(m);
            regs_.write(d.rd, static_cast<int32_t>(r));
            break;
        }

        case Format::FP_CMP: {
            fpu_.fcmp(static_cast<uint32_t>(regs_.read(d.rs1)),
                      static_cast<uint32_t>(regs_.read(d.rs2)));
            regs_.set_flags(fpu_.getE(), fpu_.getGT(), fpu_.getST());
            break;
        }

        case Format::FP_R1: {
            const uint32_t a = static_cast<uint32_t>(regs_.read(d.rs1));
            uint32_t r = 0;
            if      (m == "sqrt") r = fpu_.sqrt(a);
            else if (m == "fabs") r = fpu_.fabs(a);
            else if (m == "sin")  r = fpu_.sin(a);
            else if (m == "cos")  r = fpu_.cos(a);
            else not_implemented(m);
            regs_.write(d.rd, static_cast<int32_t>(r));
            break;
        }

        case Format::NO_OPERAND: {
            if (m != "nop") not_implemented(m);
            break;
        }

        default:
            // vector instructions: not wired in yet.
            not_implemented(m);
    }
    return next;
}

}  // namespace risc201
