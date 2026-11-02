#include "Simulator/program_loader.h"

#include <fstream>
#include <ostream>
#include <stdexcept>

#include "Assembler/risc201_common.h"

namespace risc201 {

std::vector<uint64_t> AssemblyResult::words() const {
    std::vector<uint64_t> out;
    out.reserve(instructions.size());
    for (const auto& inst : instructions) out.push_back(inst.word);
    return out;
}

AssemblyResult assemble(const std::vector<std::string>& source, std::ostream& err) {
    std::vector<Token> tokens = tokenize(source);                  // stage 1
    try {
        tokens = expand_stack_macros(tokens);                      // stack macros (pushm / popm)
    } catch (const std::exception& e) {
        err << e.what() << "\n";
        AssemblyResult bad;
        bad.error_count = 1;
        return bad;
    }
    auto symbols = build_symbol_table(tokens);                     // stage 2: pass 1

    AssemblyResult result;
    for (const auto& tok : tokens) {                               // stage 3: pass 2
        if (tok.mnemonic.empty()) continue;  // label-only line
        // Encode one instruction at a time so a bad line reports its line
        // number and assembly can continue past it.
        try {
            result.instructions.push_back(
                {tok.address, tok.line_number, encode_instruction(tok, symbols)});
        } catch (const std::exception& e) {
            err << "Line " << tok.line_number << ": error: " << e.what() << "\n";
            ++result.error_count;
        }
    }
    return result;
}

std::vector<std::string> read_source_lines(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Could not open input file: " + path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    return lines;
}

AssemblyResult assemble_file(const std::string& path, std::ostream& err) {
    return assemble(read_source_lines(path), err);
}

}  // namespace risc201
