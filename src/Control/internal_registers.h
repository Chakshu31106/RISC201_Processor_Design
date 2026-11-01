
#ifndef INTERNAL_REGISTERS_H
#define INTERNAL_REGISTERS_H

#include <cstddef>

namespace control {

enum class InternalRegister : std::size_t {
    PC = 0,
    IR,
    I,
    RD,RS1,RS2,

    IMMX,
    BRANCH_TARGET,
    BIT_IDX,

    VD,VS1,VS2,
    
    REG_SRC,REG_DATA,REG_VAL,
    
    A,B,ALU_RESULT,
    
    FLAG_E,FLAG_GT,FLAG_ST,
    
    MAR,MDR,
    
    LD_RESULT,
    
    A_F,B_F,F_RESULT,
    
    TMP1,TMP2,TMP3,TMP4,

    COUNT
};

constexpr std::size_t registerIndex(InternalRegister reg) {
    return static_cast<std::size_t>(reg);
}

constexpr std::size_t INTERNAL_REGISTER_COUNT =
    registerIndex(InternalRegister::COUNT);

static_assert(INTERNAL_REGISTER_COUNT == 31,
              "unexpected number of internal registers");

static_assert(INTERNAL_REGISTER_COUNT <= 32,
              "5 bits are insufficient for internal register indices");

}

#endif 






