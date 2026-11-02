// Pipeline timing model (statistics only, nothing is emulated cycle by cycle).
//
// Adapted from the team's isa_pipeline_sim.cpp: every instruction the simulator really executed is
// fed, in order, to a timing model of the 4-stage (IF OF EX MA+RW) and 6-stage (IF ID RR EX MA RW)
// pipelines, which counts stalls, flushes, CPI, execution time and speedup.
//
//   * RAW hazard, no forwarding : the operand-read stage waits until the producer's write-back stage.
//   * RAW hazard, forwarding    : EX waits until the producer's EX result (ALU) or MA result (load) is ready.
//   * Branches: static predict-not-taken, resolved in EX. A taken branch / b / call / ret flushes the
//     instructions fetched in the stages before EX and fetch restarts the cycle after EX.
#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "Simulator/decoder.h"

namespace risc201 {

// What the timing model needs to know about one executed instruction.
struct TraceEntry {
    uint32_t src = 0;        // bit r set: the instruction reads register r
    uint32_t dst = 0;        // bit r set: the instruction writes register r (r29 / ip excluded)
    uint8_t  ctrl = 0;       // 0 none, 1 conditional branch not taken, 2 conditional taken, 3 unconditional (b/call/ret)
    bool     load = false;   // data comes from memory (ld, vld): the result is ready only after MA
};

// Builds the entry for a decoded instruction. `taken` says whether the ip left the sequential path.
TraceEntry make_trace_entry(const DecodedInstr& d, bool taken);

struct PipeStats {
    std::string name;
    double period_ns = 0;
    long long instructions = 0, cycles = 0, stalls = 0, flushes = 0, flushed_slots = 0,
              flush_cycles = 0, branches = 0, mispredictions = 0;
    double cpi() const { return instructions ? static_cast<double>(cycles) / instructions : 0.0; }
    double time_ns() const { return cycles * period_ns; }
};

// Runs the trace through both pipelines.
void run_pipeline_model(const std::vector<TraceEntry>& trace, bool forwarding, PipeStats& four, PipeStats& six);

// Prints the comparison table (same layout as the team's tool).
void print_pipeline_stats(std::ostream& os, const PipeStats& four, const PipeStats& six, bool forwarding);

}  // namespace risc201
