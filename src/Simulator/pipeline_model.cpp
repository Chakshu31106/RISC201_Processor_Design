#include "Simulator/pipeline_model.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ostream>

namespace risc201 {

namespace {

uint32_t bit(int r) { return (r >= 0 && r < 32) ? (1u << r) : 0u; }

// Vector id (0, 1, 2) -> the three registers r20-r22 / r23-r25 / r26-r28.
uint32_t vec_regs(int id) { return (id >= 0 && id <= 2) ? (7u << (20 + 3 * id)) : 0u; }

}  // namespace

TraceEntry make_trace_entry(const DecodedInstr& d, bool taken) {
    TraceEntry t;
    const std::string& m = d.mnemonic;
    const uint32_t SP = bit(30), RA = bit(31);

    switch (d.format) {
        case Format::R:
        case Format::R_IMM:
            t.dst = bit(d.rd);
            t.src = bit(d.rs1) | (d.use_imm ? 0u : bit(d.rs2));
            break;
        case Format::CMP:
            t.src = bit(d.rs1) | (d.use_imm ? 0u : bit(d.rs2));
            break;
        case Format::R1:
        case Format::FP_R1:
        case Format::BIT:
            t.dst = bit(d.rd);
            t.src = bit(d.rs1);
            break;
        case Format::R1S:
            t.dst = t.src = bit(d.rs1);
            break;
        case Format::BRANCH:
            if (m == "call") { t.dst = RA | SP; t.src = SP | RA; }   // call pushes ra and the return address
            t.ctrl = (m == "b" || m == "call") ? 3 : (taken ? 2 : 1);
            break;
        case Format::BRANCH_NOOP:   // ret: pops the return address and the saved ra
            t.src = SP;
            t.dst = SP | RA;
            t.ctrl = 3;
            break;
        case Format::MEM_LOAD:
            t.dst = bit(d.rd);
            t.src = bit(d.rs1);
            t.load = true;
            break;
        case Format::MEM_STORE:     // st rs1(value), offset[rd(base)]
            t.src = bit(d.rd) | bit(d.rs1);
            break;
        case Format::MOVE:
            t.dst = bit(d.rd);
            t.src = d.use_imm ? 0u : bit(d.rs2);
            if (d.mode == 2) t.src |= bit(d.rd);   // movh keeps the low half of rd
            break;
        case Format::STACK_1:
            if (m == "push")      { t.src = bit(d.rd) | SP; t.dst = SP; }
            else if (m == "pop")  { t.src = SP;             t.dst = bit(d.rd) | SP; }
            else /* peek */       { t.src = SP;             t.dst = bit(d.rd); }
            break;
        case Format::STACK_N_COUNT:  // mpop #n
            t.src = t.dst = SP;
            break;
        case Format::STACK_N_NOOP:   // clr
            t.dst = SP;
            break;
        case Format::FP_R:
            t.dst = bit(d.rd);
            t.src = bit(d.rs1) | bit(d.rs2);
            break;
        case Format::FP_CMP:
            t.src = bit(d.rs1) | bit(d.rs2);
            break;
        case Format::VEC_3REG:
            t.dst = vec_regs(d.rd);
            t.src = vec_regs(d.rs1) | vec_regs(d.rs2);
            break;
        case Format::VEC_SCALAR2:
            t.dst = bit(d.rd);
            t.src = vec_regs(d.rs1) | vec_regs(d.rs2);
            break;
        case Format::VEC_SCALAR1:
            t.dst = bit(d.rd);
            t.src = vec_regs(d.rs1);
            break;
        case Format::VEC_LOAD:
            t.dst = vec_regs(d.rd);
            t.src = bit(d.rs1);
            t.load = true;
            break;
        case Format::VEC_STORE:      // field rd = base register, field rs1 = vector id
            t.src = bit(d.rd) | vec_regs(d.rs1);
            break;
        case Format::NO_OPERAND:
            break;
    }
    t.dst &= ~bit(29);   // r29 (ip) is not an ordinary destination
    return t;
}

namespace {

class Pipe {
public:
    Pipe(std::string name, std::vector<double> delays, int rd_stage, int ex, int ma, int wb, bool fwd)
        : stats_{}, d_(std::move(delays)), n_(static_cast<int>(d_.size())), R_(rd_stage), EX_(ex), MA_(ma), W_(wb),
          fwd_(fwd), prev_(d_.size(), -1) {
        stats_.name = std::move(name);
        stats_.period_ns = *std::max_element(d_.begin(), d_.end());
        std::fill(std::begin(ready_), std::end(ready_), 0LL);
    }

    void feed(const TraceEntry& I) {
        std::vector<long long> e(n_);
        const int hazard_stage = fwd_ ? EX_ : R_;
        for (int s = 0; s < n_; ++s) {
            long long t = s ? e[s - 1] + 1 : redirect_;
            t = std::max(t, s < n_ - 1 ? prev_[s + 1] : prev_[s] + 1);   // wait until the next stage is free
            if (s == hazard_stage) {
                long long h = 0;
                for (int r = 0; r < 32; ++r)
                    if (I.src >> r & 1) h = std::max(h, ready_[r]);
                if (h > t) { stats_.stalls += h - t; t = h; }          // data-hazard stall cycles
            }
            e[s] = t;
        }
        redirect_ = 0;
        for (int r = 0; r < 32; ++r)
            if (I.dst >> r & 1) ready_[r] = fwd_ ? e[I.load ? MA_ : EX_] + 1 : e[W_];
        if (I.ctrl) {
            ++stats_.branches;
            if (I.ctrl >= 2) {
                ++stats_.flushes;
                stats_.flushed_slots += EX_;
                redirect_ = e[EX_] + 1;
                stats_.flush_cycles += std::max(0LL, redirect_ - e[1]);
            }
            if (I.ctrl == 2) ++stats_.mispredictions;
        }
        prev_ = e;
        last_ = e[n_ - 1];
        ++stats_.instructions;
        stats_.cycles = last_ + 1;
    }

    const PipeStats& stats() const { return stats_; }

private:
    PipeStats stats_;
    std::vector<double> d_;
    int n_, R_, EX_, MA_, W_;
    bool fwd_;
    std::vector<long long> prev_;
    long long redirect_ = 0, last_ = -1;
    long long ready_[32];
};

}  // namespace

void run_pipeline_model(const std::vector<TraceEntry>& trace, bool forwarding, PipeStats& four, PipeStats& six) {
    // name, stage delays (ns), {operand-read stage, EX stage, MA stage, write-back stage}
    Pipe p4("4-stage", {3.0, 3.0, 3.0, 4.0}, 1, 2, 3, 3, forwarding);                 // IF OF EX MA+RW
    Pipe p6("6-stage", {3.0, 1.5, 1.5, 3.0, 3.0, 1.0}, 2, 3, 4, 5, forwarding);       // IF ID RR EX MA RW
    for (const TraceEntry& t : trace) { p4.feed(t); p6.feed(t); }
    four = p4.stats();
    six = p6.stats();
    if (trace.empty()) { four.cycles = six.cycles = 0; }
}

void print_pipeline_stats(std::ostream& os, const PipeStats& a, const PipeStats& b, bool forwarding) {
    char line[160];
    os << "Forwarding: " << (forwarding ? "ON" : "OFF")
       << " | predictor: static not-taken, branch resolved in EX\n\n";
    std::snprintf(line, sizeof line, "%-26s %14s %14s\n", "Metric", a.name.c_str(), b.name.c_str());
    os << line;
    auto row = [&](const char* label, double x, double y, const char* fmt) {
        char xa[64], xb[64];
        std::snprintf(xa, sizeof xa, fmt, x);
        std::snprintf(xb, sizeof xb, fmt, y);
        std::snprintf(line, sizeof line, "%-26s %14s %14s\n", label, xa, xb);
        os << line;
    };
    row("Total instructions", a.instructions, b.instructions, "%.0f");
    row("Clock cycles", static_cast<double>(a.cycles), static_cast<double>(b.cycles), "%.0f");
    row("Clock period (ns)", a.period_ns, b.period_ns, "%.2f");
    row("Stall cycles (data)", static_cast<double>(a.stalls), static_cast<double>(b.stalls), "%.0f");
    row("Flushes (events)", static_cast<double>(a.flushes), static_cast<double>(b.flushes), "%.0f");
    row("Flushed instr. slots", static_cast<double>(a.flushed_slots), static_cast<double>(b.flushed_slots), "%.0f");
    row("Flush penalty cycles", static_cast<double>(a.flush_cycles), static_cast<double>(b.flush_cycles), "%.0f");
    row("Branches/jumps executed", static_cast<double>(a.branches), static_cast<double>(b.branches), "%.0f");
    row("Branch mispredictions", static_cast<double>(a.mispredictions), static_cast<double>(b.mispredictions), "%.0f");
    row("CPI", a.cpi(), b.cpi(), "%.3f");
    row("Execution time (ns)", a.time_ns(), b.time_ns(), "%.1f");
    const double t4 = a.time_ns(), t6 = b.time_ns();
    if (t4 > 0 && t6 > 0) {
        std::snprintf(line, sizeof line, "\nSpeedup (6-stage over 4-stage) = T4/T6 = %.3f\n", t4 / t6);
        os << line;
        std::snprintf(line, sizeof line, "%s is faster by %.1f%%\n", t6 < t4 ? "6-stage" : "4-stage",
                      std::fabs(t4 - t6) / std::max(t4, t6) * 100);
        os << line;
    }
}

}  // namespace risc201
