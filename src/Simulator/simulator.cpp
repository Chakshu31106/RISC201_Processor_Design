#include "Simulator/simulator.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "Control/control_unit.h"
#include "Control/microcode_setup.h"
#include "Simulator/program_loader.h"
#include "Assembler/risc201_common.h"

namespace risc201 {

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

static bool ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool load_program_file(CPU& cpu, const std::string& path, std::ostream& err) {
    try {
        if (ends_with(path, ".asm")) {
            AssemblyResult result = assemble_file(path, err);
            if (!result.ok()) return false;
            cpu.load_program(result.words());
        } else {
            cpu.load_hex(path);
        }
    } catch (const std::exception& e) {
        err << "Error: " << e.what() << "\n";
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Debugger
// ---------------------------------------------------------------------------

static std::string hex_addr(uint32_t a) {
    std::ostringstream o;
    o << "0x" << std::hex << std::setw(4) << std::setfill('0') << a;
    return o.str();
}

// Accepts decimal or 0x-prefixed hex.
static bool parse_number(const std::string& s, uint64_t& value) {
    try {
        size_t used = 0;
        value = std::stoull(s, &used, 0);
        return used == s.size();
    } catch (...) {
        return false;
    }
}

static void show_next(const CPU& cpu, std::ostream& out) {
    uint32_t ip = static_cast<uint32_t>(cpu.regs().ip());
    if (cpu.imem().in_range(ip))
        out << "  next: " << hex_addr(ip) << "  " << disassemble_one(cpu.imem().fetch(ip)) << "\n";
}

static void report(const CPU& cpu, std::ostream& out) {
    uint32_t ip = static_cast<uint32_t>(cpu.regs().ip());
    switch (cpu.last_stop()) {
        case StopReason::None:
            show_next(cpu, out);
            break;
        case StopReason::EndOfProgram:
            out << "Program finished after " << cpu.steps() << " steps.";
            out << "\n";
            break;
        case StopReason::Error:
            out << "Error: " << cpu.message() << "\n";
            break;
        case StopReason::NotImplemented:
            out << "Stopped: " << cpu.message() << "\n";
            break;
        case StopReason::MaxSteps:
            out << "Stopped: step limit reached (" << cpu.steps() << " steps so far).\n";
            show_next(cpu, out);
            break;
        case StopReason::Breakpoint:
            out << "Breakpoint hit at " << hex_addr(ip) << ".\n";
            show_next(cpu, out);
            break;
    }
}

static std::string hex32(uint32_t v) {
    std::ostringstream o;
    o << "0x" << std::hex << std::setw(8) << std::setfill('0') << v;
    return o.str();
}

// ASCII picture of the stack: it starts at the top of data memory (0x10000) and
// grows down, so the newest word (at sp) is drawn on top.
void show_stack(const CPU& cpu, std::ostream& out, uint32_t max_words) {
    const uint32_t sp = static_cast<uint32_t>(cpu.regs().sp());
    if (sp % 4 != 0 || sp > CPU::kStackTop || sp > cpu.dmem().size()) {
        out << "Stack: sp = " << hex_addr(sp) << " is outside data memory!\n";
        return;
    }
    const uint32_t depth = (CPU::kStackTop - sp) / 4;
    out << "Stack: " << depth << " words used   sp = " << hex_addr(sp)
        << "   (starts at 0x" << std::hex << CPU::kStackTop << std::dec << ", grows down)\n";
    out << "          +------------+\n";
    if (depth == 0) out << "          |   (empty)  |\n";
    uint32_t shown = depth < max_words ? depth : max_words;
    for (uint32_t i = 0; i < shown; ++i) {
        uint32_t addr = sp + i * 4;
        out << "   " << hex_addr(addr) << " | " << hex32(cpu.dmem().readWord(addr)) << " |"
            << (display_format() == NumFormat::Int ? "" : "  float " + float_text(static_cast<int32_t>(cpu.dmem().readWord(addr))))
            << (i == 0 ? " <- top (sp)" : "") << "\n";
    }
    if (shown < depth) out << "          |    ...     |  (" << depth - shown << " more below)\n";
    out << "          +------------+  bottom = 0x" << std::hex << CPU::kStackTop << std::dec << "\n";
}

static void show_memory(const CPU& cpu, uint32_t addr, uint32_t words, std::ostream& out) {
    for (uint32_t i = 0; i < words; ++i, addr += 4) {
        if (addr + 4 > cpu.dmem().size()) { out << "  (end of data memory)\n"; return; }
        uint32_t v = cpu.dmem().readWord(addr);
        out << "  " << hex_addr(addr) << "  " << hex32(v);
        if (display_format() != NumFormat::Float) out << "  " << static_cast<int32_t>(v);
        if (display_format() != NumFormat::Int) out << "  float " << float_text(static_cast<int32_t>(v));
        out << "\n";
    }
}

// ---------------------------------------------------------------------------
// Stage view: shows ONE instruction going through IF -> ID -> EX -> WB.
// This is a display only (instructions do not overlap). The instruction is
// really executed on the WB step; EX shows its result using a throw-away copy.
// ---------------------------------------------------------------------------

// Stage names follow the team's pipeline documents.
//   4-stage: IF, OF (decode + read operands), EX, MA+RW (memory access then register write-back)
//   6-stage: IF, ID (decode), RR (read registers), EX, MA (memory access), RW (register write-back)
static const std::vector<std::string>& stage_names(int mode) {
    static const std::vector<std::string> four = {"IF", "OF", "EX", "MA+RW"};
    static const std::vector<std::string> six = {"IF", "ID", "RR", "EX", "MA", "RW"};
    return mode == 6 ? six : four;
}

// Describes what changed between two CPU states. `regs` / `mem` select which parts.
static std::string describe_changes(const CPU& before, const CPU& after, bool regs = true, bool mem = true) {
    std::ostringstream o;
    bool any = false;
    auto sep = [&]() { o << (any ? ", " : ""); any = true; };
    if (regs) {
        for (int i = 0; i < RegisterFile::NUM_REGS; ++i) {
            if (i == RegisterFile::IP) continue;
            int32_t a = before.regs().read(static_cast<uint8_t>(i));
            int32_t b = after.regs().read(static_cast<uint8_t>(i));
            if (a != b) { sep(); o << "r" << i << ": " << value_text(a) << " -> " << value_text(b); }
        }
        if (before.regs().flag_e() != after.regs().flag_e() || before.regs().flag_gt() != after.regs().flag_gt() ||
            before.regs().flag_st() != after.regs().flag_st()) {
            sep();
            o << "flags E=" << after.regs().flag_e() << " GT=" << after.regs().flag_gt() << " ST=" << after.regs().flag_st();
        }
    }
    if (mem) {
        int shown = 0;
        for (uint32_t addr = 0; addr + 4 <= after.dmem().size(); addr += 4) {
            uint32_t a = before.dmem().readWord(addr), b = after.dmem().readWord(addr);
            if (a != b && shown++ < 4) { sep(); o << "mem[" << hex_addr(addr) << "]: " << value_text(static_cast<int32_t>(a)) << " -> " << value_text(static_cast<int32_t>(b)); }
        }
    }
    if (!any) o << (regs && mem ? "no register or memory change" : mem ? "no data-memory access" : "no register change");
    return o.str();
}

// Advances `stage` (index into stage_names(mode)) by one. Returns the next stage index.
// The instruction really commits on the LAST stage (WB-like): MA+RW in the 4-stage
// view, RW in the 6-stage view.
static int advance_stage(CPU& cpu, int mode, int stage, std::ostream& out) {
    const std::vector<std::string>& names = stage_names(mode);
    const int last = static_cast<int>(names.size()) - 1;
    if (cpu.halted()) { report(cpu, out); return 0; }
    const uint32_t ip = static_cast<uint32_t>(cpu.regs().ip());
    if (!cpu.imem().in_range(ip)) { cpu.step(); report(cpu, out); return 0; }
    const uint64_t word = cpu.imem().fetch(ip);
    const std::string& name = names[static_cast<std::size_t>(stage)];
    out << "[" << name << "] ";

    auto fields = [&](const DecodedInstr& d, bool with_values) {
        std::ostringstream f;
        f << "rd=" << int(d.rd) << " rs1=" << int(d.rs1) << " rs2=" << int(d.rs2) << " imm=" << d.imm;
        if (with_values) f << "   values: rs1=" << cpu.regs().read(d.rs1) << " rs2=" << cpu.regs().read(d.rs2);
        return f.str();
    };
    auto fail_if_bad = [&](const CPU& preview) {
        if (preview.last_stop() == StopReason::Error || preview.last_stop() == StopReason::NotImplemented) {
            out << preview.message() << "\n";
            cpu.step();                       // really halt, same as `step`
            report(cpu, out);
            return true;
        }
        return false;
    };

    if (stage == 0) {
        out << "fetch: ip = " << hex_addr(ip) << ", word = 0x" << std::hex << std::setw(16) << std::setfill('0')
            << word << std::dec << "   (next ip = ip + 8)\n";
        return 1;
    }
    if (stage == 1) {
        DecodedInstr d = decode(word);
        if (!d.valid) { out << "unknown opcode\n"; return 2; }
        out << (mode == 6 ? "decode: " : "decode and read operands: ") << disassemble_one(word) << "   opcode 0x" << std::hex
            << int(d.opcode) << std::dec << "  " << fields(d, mode != 6) << "\n";
        return 2;
    }
    if (mode == 6 && stage == 2) {
        DecodedInstr d = decode(word);
        out << "read registers: rs1 (r" << int(d.rs1) << ") = " << cpu.regs().read(d.rs1) << ", rs2 (r" << int(d.rs2)
            << ") = " << cpu.regs().read(d.rs2) << "\n";
        return 3;
    }
    const int ex = mode == 6 ? 3 : 2;
    if (stage == ex) {
        CPU preview = cpu;
        preview.set_trace(nullptr);
        preview.step();
        if (fail_if_bad(preview)) return 0;
        out << "execute: result " << describe_changes(cpu, preview, true, false) << "\n";
        return stage + 1;
    }
    if (mode == 6 && stage == 4) {
        CPU preview = cpu;
        preview.set_trace(nullptr);
        preview.step();
        out << "memory access: " << describe_changes(cpu, preview, false, true) << "\n";
        return 5;
    }
    // Last stage: commit for real.
    CPU before = cpu;
    before.set_trace(nullptr);
    cpu.step();
    out << (mode == 6 ? "write-back: " : "memory access and write-back: ")
        << describe_changes(before, cpu, true, mode != 6) << ";  ip = "
        << hex_addr(static_cast<uint32_t>(cpu.regs().ip())) << "\n";
    (void)last;
    report(cpu, out);
    return 0;
}

// ---------------------------------------------------------------------------
// Micro-PC view: runs the NEXT instruction through the microprogrammed
// ControlUnit, one microinstruction at a time, on a private copy of the
// registers and data memory. It is a preview: the real CPU is not changed
// (use `step` to execute the instruction for real).
// ---------------------------------------------------------------------------

static const char* const kMicroTypes[] = {"MLOAD_IR", "MDECODE", "MSWITCH", "MMOV", "MMOVI", "MADD", "MB", "MBEQ"};
static const char* const kUnits[] = {"-", "ALU", "REGFILE", "MEMORY", "FPU", "VECTOR"};
static const char* const kInternal[] = {
    "PC", "IR", "I", "RD", "RS1", "RS2", "IMMX", "BRANCH_TARGET", "BIT_IDX", "VD", "VS1", "VS2",
    "REG_SRC", "REG_DATA", "REG_VAL", "A", "B", "ALU_RESULT", "FLAG_E", "FLAG_GT", "FLAG_ST",
    "MAR", "MDR", "LD_RESULT", "A_F", "B_F", "F_RESULT", "TMP1", "TMP2", "TMP3", "TMP4"};

static void show_micro(const CPU& cpu, std::ostream& out, uint32_t max_steps = 200) {
    const uint32_t ip = static_cast<uint32_t>(cpu.regs().ip());
    if (cpu.halted() || !cpu.imem().in_range(ip)) { out << "No instruction to run.\n"; return; }
    const uint64_t word = cpu.imem().fetch(ip);

    try {
        control::ControlUnit cu;
        cu.loadProgram(control::buildDefaultMicrocode());
        for (int i = 0; i < RegisterFile::NUM_REGS; ++i)
            cu.writeRegister(static_cast<std::size_t>(i), cpu.regs().read(static_cast<uint8_t>(i)));
        cu.writeInternal(control::InternalRegister::FLAG_E, cpu.regs().flag_e());
        cu.writeInternal(control::InternalRegister::FLAG_GT, cpu.regs().flag_gt());
        cu.writeInternal(control::InternalRegister::FLAG_ST, cpu.regs().flag_st());
        for (uint32_t a = 0; a + 4 <= cpu.dmem().size(); a += 4) {
            uint32_t v = cpu.dmem().readWord(a);
            if (v) cu.writeMemoryWord(a, v);
        }
        cu.loadInstruction(word, ip);

        out << "Microcode for " << hex_addr(ip) << "  " << disassemble_one(word) << "\n";
        out << "  uPC  type      src dest  uimm  seq  unit     opcode | internal registers changed\n";
        auto snapshot = [&]() {
            std::vector<int64_t> v;
            for (std::size_t i = 0; i < control::INTERNAL_REGISTER_COUNT; ++i)
                v.push_back(cu.readInternal(static_cast<control::InternalRegister>(i)));
            return v;
        };
        uint32_t steps = 0;
        while (steps < max_steps) {
            const int upc = cu.getMicroPC();
            if (upc < 0) break;
            const control::Microinstruction& mi = cu.getProgram().controlMemory[static_cast<std::size_t>(upc)];
            const std::vector<int64_t> before = snapshot();
            std::ostringstream row;
            row << "  " << std::setw(4) << upc << " " << std::left << std::setw(9) << kMicroTypes[static_cast<int>(mi.type)]
                << std::right << std::setw(4) << mi.src << std::setw(5) << mi.dest << std::setw(6) << mi.uimm
                << std::setw(5) << mi.seqAddr << "  " << std::left << std::setw(8) << kUnits[static_cast<int>(mi.unit)]
                << std::right << std::setw(6) << mi.opcode << " |";
            if (!cu.stepMicroinstruction()) break;
            ++steps;
            const std::vector<int64_t> after = snapshot();
            for (std::size_t i = 0; i < after.size(); ++i)
                if (after[i] != before[i]) row << " " << kInternal[i] << ": " << before[i] << " -> " << after[i] << ";";
            out << row.str() << "\n";
        }
        out << "  " << steps << " microinstructions.  ";
        bool any = false;
        for (int i = 0; i < RegisterFile::NUM_REGS; ++i) {
            if (i == RegisterFile::IP) continue;
            int32_t a = cpu.regs().read(static_cast<uint8_t>(i)), b = cu.readRegister(static_cast<std::size_t>(i));
            if (a != b) { out << (any ? ", " : "Registers: ") << "r" << i << ": " << value_text(a) << " -> " << value_text(b); any = true; }
        }
        int shown = 0;
        for (uint32_t a = 0; a + 4 <= cpu.dmem().size(); a += 4) {
            uint32_t was = cpu.dmem().readWord(a), now = cu.readMemoryWord(a);
            if (was != now && shown++ < 4) { out << (any ? ", " : "Memory: ") << "mem[" << hex_addr(a) << "]: " << value_text(static_cast<int32_t>(was)) << " -> " << value_text(static_cast<int32_t>(now)); any = true; }
        }
        if (!any) out << "No register or memory change.";
        out << "\n  (preview only -- use `step` to execute it)\n";
    } catch (const std::exception& e) {
        out << "  Control unit error: " << e.what() << "\n";
    }
}

static void print_help(std::ostream& out) {
    out <<
        "Commands:\n"
        "  load <file>        load a .asm (assembled) or .hex file\n"
        "  step [n]   (s)     execute n instructions (default 1)\n"
        "  run [max]  (r)     run until halt/breakpoint (default limit 1000000 steps)\n"
        "  continue   (c)     same as run\n"
        "  regs               show all registers and flags\n"
        "  reg <n>            show one register (0-31)\n"
        "  list [n]   (l)     disassemble n instructions from ip (default 5)\n"
        "  break <addr>       set a breakpoint (decimal or 0x hex byte address)\n"
        "  unbreak <addr>     remove a breakpoint\n"
        "  breaks             list breakpoints\n"
        "  trace on|off       print every executed instruction\n"
        "  stage      (sg)    advance ONE stage of the next instruction (see `pipeline`)\n"
        "  pipeline 4|6       choose the stage view: 4 = IF OF EX MA+RW, 6 = IF ID RR EX MA RW\n"
        "  perf [fwd]         pipeline statistics (stalls, flushes, CPI, speedup) for the instructions\n"
        "                     executed so far; `perf fwd` assumes operand forwarding\n"
        "  micro              preview the next instruction microinstruction by microinstruction\n"
        "  stack              draw the stack (starts at the top of data memory, grows down)\n"
        "  mem <addr> [n]     show n data-memory words from addr (default 4; addr multiple of 4)\n"
        "  fmt int|float|both choose how registers and memory are shown (default both: integer and float)\n"
        "  reset              reset registers and ip\n"
        "  help               this text\n"
        "  quit       (q)     exit\n";
}

void run_cli(CPU& cpu, std::istream& in, std::ostream& out) {
    out << "RISC201 simulator -- type 'help' for commands.\n";

    int pipeline = 4;   // 4-stage or 6-stage view
    int stage = 0;      // which stage the `stage` command shows next
    std::string line;
    while (true) {
        out << "risc201> " << std::flush;
        if (!std::getline(in, line)) break;

        std::istringstream ss(line);
        std::string cmd;
        if (!(ss >> cmd)) continue;
        std::vector<std::string> args;
        for (std::string a; ss >> a;) args.push_back(a);

        uint64_t n = 0;

        if (cmd == "quit" || cmd == "q" || cmd == "exit") {
            break;

        } else if (cmd == "help" || cmd == "h" || cmd == "?") {
            print_help(out);

        } else if (cmd == "load") {
            if (args.size() != 1) { out << "usage: load <file>\n"; continue; }
            stage = 0;
            if (load_program_file(cpu, args[0], out))
                out << "Loaded " << cpu.imem().size_words() << " instructions from " << args[0] << ".\n";

        } else if (cmd == "step" || cmd == "s") {
            uint64_t count = 1;
            if (!args.empty() && !parse_number(args[0], count)) { out << "usage: step [n]\n"; continue; }
            stage = 0;
            for (uint64_t i = 0; i < count && !cpu.halted(); ++i) cpu.step();
            report(cpu, out);

        } else if (cmd == "run" || cmd == "r" || cmd == "continue" || cmd == "c") {
            uint64_t max_steps = 1000000;
            if (!args.empty() && !parse_number(args[0], max_steps)) { out << "usage: run [max]\n"; continue; }
            stage = 0;
            cpu.run(max_steps);
            report(cpu, out);

        } else if (cmd == "perf") {
            if (args.size() > 1 || (args.size() == 1 && args[0] != "fwd")) {
                out << "usage: perf [fwd]\n";
            } else if (cpu.pipe_trace().empty()) {
                out << "No instructions executed yet. Run or step the program first.\n";
            } else {
                PipeStats four, six;
                const bool fwd = !args.empty();
                run_pipeline_model(cpu.pipe_trace(), fwd, four, six);
                print_pipeline_stats(out, four, six, fwd);
                if (cpu.pipe_trace_truncated())
                    out << "\n(note: only the first " << CPU::kMaxPipeTrace << " instructions are included)\n";
            }

        } else if (cmd == "regs") {
            out << cpu.regs().dump(display_format());

        } else if (cmd == "reg") {
            if (args.size() != 1 || !parse_number(args[0], n) || n >= RegisterFile::NUM_REGS) {
                out << "usage: reg <0-31>\n";
                continue;
            }
            int32_t v = cpu.regs().read(static_cast<uint8_t>(n));
            out << "r" << n << " = " << value_text(v) << "  (0x" << std::hex << static_cast<uint32_t>(v) << std::dec << ")\n";

        } else if (cmd == "list" || cmd == "l") {
            uint64_t count = 5;
            if (!args.empty() && !parse_number(args[0], count)) { out << "usage: list [n]\n"; continue; }
            uint32_t ip = static_cast<uint32_t>(cpu.regs().ip());
            uint32_t pc = ip;
            for (uint64_t i = 0; i < count && cpu.imem().in_range(pc); ++i, pc += 8) {
                out << (pc == ip ? "=> " : "   ")
                    << (cpu.breakpoints().count(pc) ? '*' : ' ')
                    << hex_addr(pc) << "  " << disassemble_one(cpu.imem().fetch(pc)) << "\n";
            }

        } else if (cmd == "break") {
            if (args.size() != 1 || !parse_number(args[0], n)) { out << "usage: break <addr>\n"; continue; }
            if (n % 8 != 0) { out << "Address must be a multiple of 8.\n"; continue; }
            cpu.add_breakpoint(static_cast<uint32_t>(n));
            out << "Breakpoint set at " << hex_addr(static_cast<uint32_t>(n)) << ".\n";

        } else if (cmd == "unbreak") {
            if (args.size() != 1 || !parse_number(args[0], n)) { out << "usage: unbreak <addr>\n"; continue; }
            cpu.remove_breakpoint(static_cast<uint32_t>(n));
            out << "Breakpoint removed at " << hex_addr(static_cast<uint32_t>(n)) << ".\n";

        } else if (cmd == "breaks") {
            if (cpu.breakpoints().empty()) out << "No breakpoints.\n";
            for (uint32_t a : cpu.breakpoints()) out << "  " << hex_addr(a) << "\n";

        } else if (cmd == "trace") {
            if (args.size() == 1 && args[0] == "on")       { cpu.set_trace(&out); out << "Trace on.\n"; }
            else if (args.size() == 1 && args[0] == "off") { cpu.set_trace(nullptr); out << "Trace off.\n"; }
            else out << "usage: trace on|off\n";

        } else if (cmd == "stage" || cmd == "sg") {
            stage = advance_stage(cpu, pipeline, stage, out);

        } else if (cmd == "micro" || cmd == "upc") {
            show_micro(cpu, out);

        } else if (cmd == "pipeline") {
            if (args.size() == 1 && (args[0] == "4" || args[0] == "6")) {
                pipeline = args[0] == "6" ? 6 : 4;
                stage = 0;
                out << pipeline << "-stage view:";
                for (const auto& n : stage_names(pipeline)) out << " " << n;
                out << "\n";
            } else out << "usage: pipeline 4|6\n";

        } else if (cmd == "stack") {
            show_stack(cpu, out);

        } else if (cmd == "mem") {
            uint64_t count = 4;
            if (args.empty() || args.size() > 2 || !parse_number(args[0], n) ||
                (args.size() == 2 && !parse_number(args[1], count)) || n % 4 != 0) {
                out << "usage: mem <addr (multiple of 4)> [words]\n";
                continue;
            }
            show_memory(cpu, static_cast<uint32_t>(n), static_cast<uint32_t>(count), out);

        } else if (cmd == "fmt" || cmd == "format") {
            if (args.size() == 1 && (args[0] == "int" || args[0] == "float" || args[0] == "both")) {
                display_format() = args[0] == "int" ? NumFormat::Int : args[0] == "float" ? NumFormat::Float : NumFormat::Both;
                out << "Display format: " << args[0] << "\n";
            } else {
                const NumFormat f = display_format();
                out << "format is " << (f == NumFormat::Int ? "int" : f == NumFormat::Float ? "float" : "both")
                    << "   usage: fmt int|float|both\n";
            }

        } else if (cmd == "reset") {
            stage = 0;
            cpu.reset();
            out << "CPU reset.\n";
            show_next(cpu, out);

        } else {
            out << "Unknown command '" << cmd << "'. Type 'help'.\n";
        }
    }

    cpu.set_trace(nullptr);   // `out` may not outlive the CPU
}

}  // namespace risc201
