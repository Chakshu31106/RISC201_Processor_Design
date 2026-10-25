#ifndef ALU_H
#define ALU_H

#include <cstdint>

class ALU
{
private:
    void eightBitCLA(
        int32_t a,
        int32_t b,
        int carryIn,
        int32_t &sum,
        int &carryOut,
        int &groupPropagate,
        int &groupGenerate);

    void blockCarryLookahead(
        int pg[4],
        int gg[4],
        int carryIn,
        int carry[5]);

    void boothShift(
        int32_t &A,
        uint32_t &Q,
        int &Qminus1);

    void restoringDivide(
        uint32_t dividend,
        uint32_t divisor,
        uint32_t &quotient,
        uint32_t &remainder);

    bool E;
    bool GT;
    bool ST;

public:
    ALU();
    // Arithmetic
    int32_t add(int32_t a, int32_t b);
    int32_t sub(int32_t a, int32_t b);
    int32_t mul(int32_t a, int32_t b);
    int32_t div(int32_t a, int32_t b);
    int32_t mod(int32_t a, int32_t b);
    int32_t cmp(int32_t a, int32_t b);
    int32_t incr(int32_t a);
    int32_t decr(int32_t a);
    int32_t pow(int32_t a, int32_t b);
    int32_t abs(int32_t a);

    // Logical
    int32_t AND(int32_t a, int32_t b);
    int32_t OR(int32_t a, int32_t b);
    int32_t NOT(int32_t a);
    int32_t NOR(int32_t a, int32_t b);
    int32_t NAND(int32_t a, int32_t b);
    int32_t XOR(int32_t a, int32_t b);

    // Shifts / rotates
    int32_t lsl(int32_t a, int32_t b);
    int32_t lsr(int32_t a, int32_t b);
    int32_t asr(int32_t a, int32_t b);
    int32_t rotl(int32_t a, int32_t b);
    int32_t rotr(int32_t a, int32_t b);

    // Bit manipulation
    int32_t pcnt(int32_t a);
    int32_t tog(int32_t a, int32_t bit);
    int32_t sbit(int32_t a, int32_t bit);
    int32_t cbit(int32_t a, int32_t bit);

    
    // Vector
    // signatures to be finalized from ISA semantics
    // to help read these value for cu in cmp instruction
    bool getE();
    bool getGT();
    bool getST();
};

#endif