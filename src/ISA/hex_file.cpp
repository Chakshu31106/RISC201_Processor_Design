#include "ISA/hex_file.h"

#include <bitset>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace risc201 {

void write_hex_file(const std::string& path, const std::vector<uint64_t>& words) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Could not open output file for writing: " + path);
    }
    for (uint64_t w : words) {
        out << std::hex << std::setw(16) << std::setfill('0') << w << "\n";
    }
    if (!out) {
        throw std::runtime_error("Failed while writing output file: " + path);
    }
}

void write_bin_file(const std::string& path, const std::vector<uint64_t>& words) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open binary output file: " + path);
    for (uint64_t w : words) out << std::bitset<64>(w) << '\n';
    if (!out) throw std::runtime_error("Failed while writing binary output file: " + path);
}

std::string bin_path_for(std::string path) {
    const auto slash = path.find_last_of("/\\");
    const auto dot = path.find_last_of('.');
    if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
        path.erase(dot);
    return path + ".bin";
}

std::vector<uint64_t> read_hex_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Could not open hex file: " + path);

    std::vector<uint64_t> words;
    std::string line;
    int line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        line = line.substr(0, line.find(';'));
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;  // blank line
        size_t end = line.find_last_not_of(" \t\r\n");
        std::string text = line.substr(start, end - start + 1);
        try {
            size_t used = 0;
            bool bin = text.size() == 64 &&
                       text.find_first_not_of("01") == std::string::npos;
            if (!bin && text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
                text.erase(0, 2);
            if (!bin && text.size() > 16) throw std::invalid_argument("too long");
            uint64_t word = std::stoull(text, &used, bin ? 2 : 16);
            if (used != text.size()) throw std::invalid_argument("trailing characters");
            words.push_back(word);
        } catch (const std::exception&) {
            throw std::runtime_error(path + ":" + std::to_string(line_no) +
                                     ": not a valid hex instruction word: " + text);
        }
    }
    return words;
}

}  // namespace risc201
