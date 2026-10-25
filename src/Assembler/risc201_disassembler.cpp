
#include "risc201_common.h"
#include <bitset>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

static uint64_t parse_word(const std::string& text) {
    if (text.empty()) {
        throw std::runtime_error("empty instruction");
    }

    bool is_binary = (text.size() == 64);

    if (is_binary) {
        for (char c : text) {
            if (c != '0' && c != '1') {
                is_binary = false;
                break;
            }
        }
    }

    size_t used = 0;
    uint64_t value;

    if (is_binary) {
        value = std::stoull(text, &used, 2);
    } else {
        std::string hex_text = text;

        if (hex_text.rfind("0x", 0) == 0 ||
            hex_text.rfind("0X", 0) == 0) {
            hex_text.erase(0, 2);
        }

        if (hex_text.empty() || hex_text.size() > 16) {
            throw std::runtime_error(
                "expected up to 16 hexadecimal digits "
                "or 64 binary digits");
        }

        value = std::stoull(hex_text, &used, 16);

        if (used != hex_text.size()) {
            throw std::runtime_error(
                "invalid hexadecimal instruction: " + text);
        }

        return value;
    }

    if (used != text.size()) {
        throw std::runtime_error(
            "invalid binary instruction: " + text);
    }

    return value;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <input.hex|input.bin>\n";
        return 1;
    }

    std::string input_path = argv[1];

    bool binary_output =
        input_path.size() >= 4 &&
        input_path.substr(input_path.size() - 4) == ".bin";

    std::ifstream in(input_path);

    if (!in) {
        std::cerr << "Could not open input file: "
                  << input_path << "\n";
        return 1;
    }

    std::string line;
    int line_number = 0;
    int errors = 0;

    while (std::getline(in, line)) {
        ++line_number;

        auto comment = line.find(';');
        if (comment != std::string::npos) {
            line.erase(comment);
        }

        auto first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            continue;
        }

        auto last = line.find_last_not_of(" \t\r\n");
        line = line.substr(first, last - first + 1);

        try {
            uint64_t word = parse_word(line);

            if (binary_output) {
                std::cout << std::bitset<64>(word);
            } else {
                std::cout << std::hex
                          << std::setw(16)
                          << std::setfill('0')
                          << word
                          << std::dec
                          << std::setfill(' ');
            }

            std::cout << "  "
                      << disassemble_one(word)
                      << "\n";

        } catch (const std::exception& e) {
            std::cerr << "Line " << line_number
                      << ": " << e.what() << "\n";
            ++errors;
        }
    }

    return errors ? 1 : 0;
}
