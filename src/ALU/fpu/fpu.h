#ifndef FPU_H
#define FPU_H

#include <cstdint>

class FPU
{
private:
    bool E;
    bool GT;
    bool ST;

    // Help to extract sign , mantissa, exponent from the bit representation
    struct FloatParts
    {
        uint32_t sign;
        uint32_t exponent;
        uint32_t fraction;
        uint32_t significand;
    };

    FloatParts unpack(uint32_t bits); // function which execute the process of extraction and store value in above variable

    uint64_t shiftRightJam(uint64_t value, unsigned int distance); // Function which helps in rounding
    struct AlignedParts
    {
        uint64_t significandA;
        uint64_t significandB;
        int32_t exponent;
    };

    AlignedParts alignExponents(FloatParts a, FloatParts b); // alignment process execution helping function

    // this will help in calculation of final significand and final sign
    struct ArithmeticResult
    {
        uint64_t significand;
        uint32_t sign;
        int32_t exponent;
    };

    ArithmeticResult addSignificands(
        const AlignedParts &aligned,
        uint32_t signA,
        uint32_t signB);

    ArithmeticResult normalize(ArithmeticResult result);   // function helps in normalisation
    ArithmeticResult roundResult(ArithmeticResult result); // store the round result
    uint32_t packResult(ArithmeticResult result);          // pack result back to 32 bit

    uint64_t multiplySignificands(
        uint32_t significandA,
        uint32_t significandB);

    int32_t multiplyExponents(
        FloatParts a,
        FloatParts b);

    struct MultiplyResult
    {
        uint64_t significand;
        int32_t exponent;
    };

    MultiplyResult normalizeProduct(
        uint64_t product,
        int32_t exponent);
    uint64_t roundRightShiftEven(uint64_t value, unsigned int shift);

    uint32_t goldschmidtReciprocal(uint32_t significandBits);
    uint32_t makeNormalizedFloat(uint32_t significand);

    uint32_t sqrtInitialGuess(uint32_t a);

public:
    FPU();

    uint32_t fadd(uint32_t a, uint32_t b);
    uint32_t fsub(uint32_t a, uint32_t b);
    uint32_t fmul(uint32_t a, uint32_t b);
    uint32_t fdiv(uint32_t a, uint32_t b);

    int32_t fcmp(uint32_t a, uint32_t b);

    uint32_t sqrt(uint32_t a);
    uint32_t fabs(uint32_t a);
    uint32_t sin(uint32_t a);
    uint32_t cos(uint32_t a);
    // Inverse cosine
    uint32_t acos(uint32_t a);

    bool getE();
    bool getGT();
    bool getST();
};

#endif