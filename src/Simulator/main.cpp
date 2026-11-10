// risc201 -- RISC201 simulator and debugger.

#include <iostream>
#include <string>

#include "Simulator/simulator.h"
#include "Simulator/cpu.h"

using namespace risc201;

static void usage(const char* prog) {
    std::cout <<
        "Usage: " << prog << " [options] [program.asm | program.hex]\n"
        "\n"
        "  With no options, starts the interactive debugger.\n"
        "  --run      run the program to completion, print the registers, exit\n"
        "  --trace    print every executed instruction (with --run)\n"
        "  --perf     after --run, print pipeline statistics (4-stage vs 6-stage)\n"
        "  --fwd      with --perf, assume operand forwarding\n"
        "  --fmt int|float|both   how registers/memory are shown (default both)\n"
        "  -h, --help show this text\n";
}

int main(int argc, char** argv) {
    bool auto_run = false;
    bool trace = false;
    bool perf = false, fwd = false;
    std::string file;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--run") auto_run = true;
        else if (a == "--trace") trace = true;
        else if (a == "--perf") perf = true;
        else if (a == "--fwd") fwd = true;
        else if (a == "--fmt" && i + 1 < argc) {
            std::string v = argv[++i];
            if (v == "int") display_format() = NumFormat::Int;
            else if (v == "float") display_format() = NumFormat::Float;
            else if (v == "both") display_format() = NumFormat::Both;
            else { std::cerr << "--fmt must be int, float or both\n"; return 1; }
        }
        else if (a == "-h" || a == "--help") { usage(argv[0]); return 0; }
        else if (!a.empty() && a[0] == '-') {
            std::cerr << "Unknown option: " << a << "\n";
            usage(argv[0]);
            return 1;
        }
        else file = a;
    }

    CPU cpu;
    if (!file.empty() && !load_program_file(cpu, file, std::cerr)) return 1;

    if (auto_run) {
        if (file.empty()) {
            std::cerr << "--run needs a program file.\n";
            return 1;
        }
        if (trace) cpu.set_trace(&std::cout);
        StopReason r = cpu.run();
        cpu.set_trace(nullptr);

        std::cout << "Stopped: " << to_string(r);
        if (!cpu.message().empty()) std::cout << " (" << cpu.message() << ")";
        std::cout << " after " << cpu.steps() << " steps.\n\n" << cpu.regs().dump(display_format());
        if (perf && !cpu.pipe_trace().empty()) {
            PipeStats four, six;
            run_pipeline_model(cpu.pipe_trace(), fwd, four, six);
            std::cout << "\n";
            print_pipeline_stats(std::cout, four, six, fwd);
        }
        return r == StopReason::EndOfProgram ? 0 : 1;
    }

    run_cli(cpu, std::cin, std::cout);
    return 0;
}
