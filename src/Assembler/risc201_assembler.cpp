#include "risc201_common.h"
#include <bitset>

static std::string binary_output_path(std::string path) {
    const auto slash = path.find_last_of("/\\");
    const auto dot = path.find_last_of('.');
    if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) {
        path.erase(dot);
    }
    return path + ".bin";
}

static void write_binary_file(const std::string& path, const std::vector<uint64_t>& words) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open binary output file: " + path);
    for (uint64_t word : words) out << std::bitset<64>(word) << '\n';
    if (!out) throw std::runtime_error("Failed while writing binary output file: " + path);
}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <input.asm> [output.hex]\n";
        return 1;
    }
    const std::string input_path = argv[1];
    const std::string output_path = argc == 3 ? argv[2] : "program.hex";
    try {
        auto source = read_source_file(input_path);
        auto tokens = tokenize(source);
        auto symbols = build_symbol_table(tokens);
        std::vector<uint64_t> words;
        int errors = 0;
        for (const auto& tok : tokens) {
            if (tok.mnemonic.empty()) continue;
            try {
                uint64_t word = encode_instruction(tok, symbols);
                words.push_back(word);
                std::cout << std::dec << tok.address << "  0x"
                          << std::hex << std::setw(16) << std::setfill('0') << word
                          << std::dec << std::setfill(' ') << "  "
                          << tok.mnemonic << "\n";
            } catch (const std::exception& e) {
                std::cerr << "Line " << tok.line_number << ": " << e.what() << "\n";
                ++errors;
            }
        }
        if (errors) {
            std::cerr << errors << " error(s); output file was not written.\n";
            return 1;
        }
        write_hex_file(output_path, words);
        const std::string binary_path = binary_output_path(output_path);
        write_binary_file(binary_path, words);
        std::cout << "Wrote " << words.size() << " instruction(s) to " << output_path << " (HEX)\n";
        std::cout << "Wrote " << words.size() << " instruction(s) to " << binary_path << " (64-bit binary text)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Assembler error: " << e.what() << "\n";
        return 1;
    }
}
