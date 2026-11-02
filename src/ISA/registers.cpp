#include "ISA/registers.h"

namespace risc201 {

std::string register_name(uint8_t num) {
    return "r" + std::to_string(num);
}

}  // namespace risc201
