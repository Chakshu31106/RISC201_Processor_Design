
#include "fpu.h"
#include <cmath>
#include <cstring>
#include <iostream>

FPU::FPU()
{
    E = false;
    GT = false;
    ST = false;
}
// Float store the number using same IEEE 754 format
uint32_t FPU::fabs(uint32_t a)
{
    return a & 0x7FFFFFFFu;
}

int32_t FPU::fcmp(uint32_t a, uint32_t b)
{
    FloatParts x = unpack(a); // created comparator x which store every unpack part of a
    FloatParts y = unpack(b); // similarly y store for b

    // NaN check
    if ((x.exponent == 255 && x.fraction != 0) ||
        (y.exponent == 255 && y.fraction != 0)) // checking if they are NAN not a number
    {
        E = false;
        GT = false;
        ST = false;
        return 0;
    }

    // +0.0 and -0.0 are equal
    if ((a & 0x7FFFFFFFu) == 0 &&
        (b & 0x7FFFFFFFu) == 0)
    {
        E = true;
        GT = false;
        ST = false;
        return 0;
    }

    // Identical bit patterns
    if (a == b)
    {
        E = true;
        GT = false;
        ST = false;
        return 0;
    }

    // Different signs: positive is greater than negative
    if (x.sign != y.sign)
    {
        GT = (x.sign == 0);
        ST = (x.sign == 1);
        E = false;
        return GT ? 1 : -1;
    }

    // Same sign: compare magnitude bits
    uint32_t magA = a & 0x7FFFFFFFu;
    uint32_t magB = b & 0x7FFFFFFFu;

    bool aGreater = magA > magB;

    if (x.sign == 0)
    {
        GT = aGreater;
        ST = !aGreater;
    }
    else
    {
        GT = !aGreater;
        ST = aGreater;
    }

    E = false;
    return GT ? 1 : -1;
}

bool FPU::getE()
{
    return E;
}

bool FPU::getGT()
{
    return GT;
}

bool FPU::getST()
{
    return ST;
}

// FADD OPERATION

FPU::FloatParts FPU::unpack(uint32_t bits)
{
    FloatParts p{};

    // Extract the sign bit.
    p.sign = (bits >> 31) & 1u;

    // Extract the 8-bit exponent.
    p.exponent = (bits >> 23) & 0xFFu;

    // Extract the 23-bit fraction.
    p.fraction = bits & 0x7FFFFFu;

    // Restore the implicit leading bit for normal numbers.
    if (p.exponent == 0)
    {
        p.significand = p.fraction;
    }
    else if (p.exponent != 255)
    {
        p.significand = (1u << 23) | p.fraction;
    }
    else
    {
        // Infinity or NaN: do not treat it as a normal number.
        p.significand = 0; // The extra exponent != 255 check is important: exponent 255 represents infinity or NaN, not an ordinary normalized number.
    }

    return p;
}

// help in alignment of exponents
FPU::AlignedParts FPU::alignExponents(FloatParts a, FloatParts b)
{
    int32_t expA = (a.exponent == 0)
                       ? -126
                       : static_cast<int32_t>(a.exponent) - 127;

    int32_t expB = (b.exponent == 0)
                       ? -126
                       : static_cast<int32_t>(b.exponent) - 127;

    uint64_t sigA = static_cast<uint64_t>(a.significand) << 3;
    uint64_t sigB = static_cast<uint64_t>(b.significand) << 3;

    if (expA < expB)
    {
        sigA = shiftRightJam(sigA,
                             static_cast<unsigned int>(expB - expA));
        expA = expB;
    }
    else if (expB < expA)
    {
        sigB = shiftRightJam(sigB,
                             static_cast<unsigned int>(expA - expB));
        expB = expA;
    }

    return {sigA, sigB, expA};
}

// Helps to in rounding of number by internally using 64 bits then chacking if the shifted out bits contain any one
uint64_t FPU::shiftRightJam(uint64_t value, unsigned int distance)
{
    if (distance == 0)
        return value;

    if (distance < 64)
    {
        uint64_t discardedMask = (1ULL << distance) - 1ULL;
        uint64_t discarded = value & discardedMask;

        value >>= distance;

        if (discarded != 0)
            value |= 1ULL;

        return value;
    }

    return (value != 0) ? 1ULL : 0ULL;
}

FPU::ArithmeticResult FPU::addSignificands(
    const AlignedParts &aligned,
    uint32_t signA,
    uint32_t signB)
{
    uint64_t sigA = aligned.significandA;
    uint64_t sigB = aligned.significandB;

    // Same signs: add magnitudes.
    if (signA == signB)
    {
        return {
            sigA + sigB,
            signA, aligned.exponent};
    }

    // Different signs: subtract the smaller magnitude from the larger.
    if (sigA > sigB)
    {
        return {
            sigA - sigB,
            signA, aligned.exponent};
    }

    if (sigB > sigA)
    {
        return {
            sigB - sigA,
            signB, aligned.exponent};
    }

    // Equal magnitudes with opposite signs produce zero.
    return {0, 0, aligned.exponent};
}

FPU::ArithmeticResult FPU::normalize(ArithmeticResult result)
{
    if (result.significand == 0)
    {
        result.exponent = -126;
        return result;
    }

    const uint64_t leadingBit = 1ULL << 26;

    // Addition may produce a carry into bit 27.
    if (result.significand >= (leadingBit << 1))
    {
        result.significand = shiftRightJam(result.significand, 1);
        result.exponent++;
    }

    // Subtraction may require left normalization.
    while (result.significand < leadingBit &&
           result.exponent > -126)
    {
        result.significand <<= 1;
        result.exponent--;
    }

    return result;
}

FPU::ArithmeticResult FPU::roundResult(ArithmeticResult result)
{
    if (result.significand == 0)
        return result;

    // Keep the 24-bit significand; the low 3 bits are G, R and S.
    uint64_t retained = result.significand >> 3;

    bool guard = (result.significand >> 2) & 1ULL;
    bool round = (result.significand >> 1) & 1ULL;
    bool sticky = result.significand & 1ULL;

    // Round to nearest, ties to even.
    bool leastSignificantBit = retained & 1ULL;
    bool increment = guard &&
                     (round || sticky || leastSignificantBit);

    if (increment)
        retained++;

    // A carry can produce 25 bits, requiring renormalization.
    if (retained >= (1ULL << 24))
    {
        retained >>= 1;
        result.exponent++;
    }

    result.significand = retained << 3;
    return result;
}

uint32_t FPU::packResult(ArithmeticResult result)
{
    // Extract the sign bit.
    uint32_t sign = result.sign & 1u;

    // Handle zero.
    if (result.significand == 0)
        return sign << 31;

    // Convert the unbiased exponent to the IEEE 754 biased exponent.
    uint32_t exponent =
        static_cast<uint32_t>(result.exponent + 127);

    // Remove the three internal rounding bits.
    uint32_t retained =
        static_cast<uint32_t>(result.significand >> 3);

    // Remove the implicit leading 1; only 23 fraction bits are stored.
    uint32_t fraction = retained & 0x7FFFFFu;

    // Assemble sign | exponent | fraction.
    return (sign << 31) |
           (exponent << 23) |
           fraction;
}

uint64_t FPU::roundRightShiftEven(uint64_t value, unsigned int shift)
{
    if (shift == 0)
        return value;

    if (shift >= 64)
        return 0;

    uint64_t quotient = value >> shift;
    uint64_t remainder = value & ((1ULL << shift) - 1ULL);
    uint64_t halfway = 1ULL << (shift - 1);

    if (remainder > halfway ||
        (remainder == halfway && (quotient & 1ULL)))
    {
        quotient++;
    }

    return quotient;
}

uint32_t FPU::makeNormalizedFloat(uint32_t significand)
{
    // A normalized significand is in [2^23, 2^24).
    return (127u << 23) | (significand & 0x7FFFFFu);
}

uint32_t FPU::fadd(uint32_t a, uint32_t b)
{
    FloatParts x = unpack(a);
    FloatParts y = unpack(b);

    const uint32_t POS_INF = 0x7F800000u;
    const uint32_t NEG_INF = 0xFF800000u;
    const uint32_t QUIET_NAN = 0x7FC00000u;

    // 1. Handle NaN.
    if ((x.exponent == 255 && x.fraction != 0) ||
        (y.exponent == 255 && y.fraction != 0))
        return QUIET_NAN;

    // 2. Handle infinity.
    if (x.exponent == 255 || y.exponent == 255)
    {
        if (x.exponent == 255 && y.exponent == 255 &&
            x.sign != y.sign)
            return QUIET_NAN;

        return (x.exponent == 255) ? a : b;
    }

    // 3. Handle zero operands.
    if ((a & 0x7FFFFFFFu) == 0)
        return b;

    if ((b & 0x7FFFFFFFu) == 0)
        return a;

    // 4. Align exponents.
    AlignedParts aligned = alignExponents(x, y);

    // 5. Add or subtract according to operand signs.
    ArithmeticResult result =
        addSignificands(aligned, x.sign, y.sign);

    // 6. Handle exact cancellation.
    if (result.significand == 0)
        return 0x00000000u;

    // 7. Normalize.
    result = normalize(result);

    // 8. Handle exponent overflow.
    if (result.exponent > 127)
        return result.sign ? NEG_INF : POS_INF;

    // 9. Handle subnormal results.
    if (result.exponent < -126)
    {
        int32_t shift = -126 - result.exponent;

        // Shift the significand right while preserving sticky bits.
        result.significand =
            shiftRightJam(result.significand,
                          static_cast<unsigned int>(shift));

        result.exponent = -126;
    }

    // 10. Round to nearest, ties to even.
    result = roundResult(result);

    // Rounding may cause exponent overflow.
    if (result.exponent > 127)
        return result.sign
                   ? 0xFF800000u
                   : 0x7F800000u;

    // 11. Pack a subnormal or zero result.
    uint64_t retained = result.significand >> 3;

    if (result.exponent == -126 &&
        retained < (1ULL << 23))
    {
        if (retained == 0)
            return result.sign << 31;

        return (result.sign << 31) |
               static_cast<uint32_t>(retained);
    }

    // 12. Pack a normal result.
    return packResult(result);
}

uint32_t FPU::fsub(uint32_t a, uint32_t b)
{
    // Flip the sign bit of B.
    uint32_t negB = b ^ 0x80000000u;

    // A - B = A + (-B).
    return fadd(a, negB);
}

uint64_t FPU::multiplySignificands(
    uint32_t significandA,
    uint32_t significandB)
{
    return static_cast<uint64_t>(significandA) *
           static_cast<uint64_t>(significandB);
}

int32_t FPU::multiplyExponents(FloatParts a, FloatParts b)
{
    int32_t expA = (a.exponent == 0)
                       ? -126
                       : static_cast<int32_t>(a.exponent) - 127;

    int32_t expB = (b.exponent == 0)
                       ? -126
                       : static_cast<int32_t>(b.exponent) - 127;

    return expA + expB;
}

FPU::MultiplyResult FPU::normalizeProduct(
    uint64_t product,
    int32_t exponent)
{
    if (product & (1ULL << 47))
    {
        return {
            shiftRightJam(product, 21),
            exponent + 1};
    }

    return {
        shiftRightJam(product, 20),
        exponent};
}

uint32_t FPU::fmul(uint32_t a, uint32_t b)
{
    FloatParts x = unpack(a);
    FloatParts y = unpack(b);

    const uint32_t POS_INF = 0x7F800000u;
    const uint32_t QUIET_NAN = 0x7FC00000u;

    uint32_t resultSign = x.sign ^ y.sign;

    // NaN
    if ((x.exponent == 255 && x.fraction != 0) ||
        (y.exponent == 255 && y.fraction != 0))
        return QUIET_NAN;

    bool xInf = x.exponent == 255;
    bool yInf = y.exponent == 255;
    bool xZero = (a & 0x7FFFFFFFu) == 0;
    bool yZero = (b & 0x7FFFFFFFu) == 0;

    // Infinity * zero
    if ((xInf && yZero) || (yInf && xZero))
        return QUIET_NAN;

    // Infinity
    if (xInf || yInf)
        return (resultSign << 31) | POS_INF;

    // Zero
    if (xZero || yZero)
        return resultSign << 31;

    // Effective exponents
    int32_t expA = (x.exponent == 0)
                       ? -126
                       : static_cast<int32_t>(x.exponent) - 127;

    int32_t expB = (y.exponent == 0)
                       ? -126
                       : static_cast<int32_t>(y.exponent) - 127;

    int32_t exponentSum = expA + expB;

    // Exact integer significand product
    uint64_t product =
        static_cast<uint64_t>(x.significand) * y.significand;

    // Find the highest set bit
    int highestBit = 63;
    while (highestBit >= 0 &&
           ((product >> highestBit) & 1ULL) == 0)
    {
        highestBit--;
    }

    if (highestBit < 0)
        return resultSign << 31;

    // Unbiased exponent of the result
    int32_t resultExponent =
        highestBit + exponentSum - 46;

    // Overflow
    if (resultExponent > 127)
        return (resultSign << 31) | POS_INF;

    // Normal result
    if (resultExponent >= -126)
    {
        int shift = highestBit - 23;
        uint64_t significand;

        if (shift > 0)
            significand = roundRightShiftEven(product, shift);
        else
            significand = product << (-shift);

        // Rounding may increase the exponent
        if (significand >= (1ULL << 24))
        {
            significand >>= 1;
            resultExponent++;
        }

        if (resultExponent > 127)
            return (resultSign << 31) | POS_INF;

        uint32_t exponentField =
            static_cast<uint32_t>(resultExponent + 127);

        uint32_t fraction =
            static_cast<uint32_t>(significand) & 0x7FFFFFu;

        return (resultSign << 31) |
               (exponentField << 23) |
               fraction;
    }

    // Subnormal result: express it in units of 2^-149
    int32_t scale = exponentSum + 103;
    uint64_t fraction;

    if (scale >= 0)
        fraction = product << scale;
    else
        fraction = roundRightShiftEven(product, -scale);

    // Rounded to zero
    if (fraction == 0)
        return resultSign << 31;

    // Rounding may produce the smallest normal number
    if (fraction >= (1ULL << 23))
        return (resultSign << 31) | 0x00800000u;

    return (resultSign << 31) |
           static_cast<uint32_t>(fraction);
}

// division implementation
uint32_t FPU::goldschmidtReciprocal(uint32_t significandBits)
{
    const uint32_t ONE = 0x3F800000u;
    const uint32_t HALF = 0x3F000000u;

    // Slide algorithm: M = P_B - 1
    uint32_t M = fsub(significandBits, ONE);

    // M' = 1 - M
    uint32_t Mprime = fsub(ONE, M);

    // Y = M' / 2
    uint32_t Y = fmul(Mprime, HALF);

    // Repeated squaring
    uint32_t Y2 = fmul(Y, Y);
    uint32_t Y4 = fmul(Y2, Y2);
    uint32_t Y8 = fmul(Y4, Y4);
    uint32_t Y16 = fmul(Y8, Y8);

    // Form factors
    uint32_t F1 = fadd(ONE, Y);
    uint32_t F2 = fadd(ONE, Y2);
    uint32_t F4 = fadd(ONE, Y4);
    uint32_t F8 = fadd(ONE, Y8);
    uint32_t F16 = fadd(ONE, Y16);

    // Product from the slide
    uint32_t result = fmul(F1, F2);
    result = fmul(result, F4);
    result = fmul(result, F8);
    result = fmul(result, F16);
    result = fmul(result, HALF);

    // One residual-based refinement
    uint32_t product = fmul(significandBits, result);
    uint32_t error = fsub(ONE, product);
    uint32_t correction = fadd(ONE, error);

    return fmul(result, correction);
}

uint32_t FPU::fdiv(uint32_t a, uint32_t b)
{
    FloatParts x = unpack(a);
    FloatParts y = unpack(b);

    const uint32_t POS_INF = 0x7F800000u;
    const uint32_t QUIET_NAN = 0x7FC00000u;

    uint32_t resultSign = x.sign ^ y.sign;

    bool xNaN = (x.exponent == 255 && x.fraction != 0);
    bool yNaN = (y.exponent == 255 && y.fraction != 0);

    bool xInf = x.exponent == 255;
    bool yInf = y.exponent == 255;

    bool xZero = (a & 0x7FFFFFFFu) == 0;
    bool yZero = (b & 0x7FFFFFFFu) == 0;

    // NaN
    if (xNaN || yNaN)
        return QUIET_NAN;

    // Infinity / infinity
    if (xInf && yInf)
        return QUIET_NAN;

    // Zero / zero
    if (xZero && yZero)
        return QUIET_NAN;

    // Infinity / finite
    if (xInf)
        return (resultSign << 31) | POS_INF;

    // Finite / infinity
    if (yInf)
        return resultSign << 31;

    // Finite / zero
    if (yZero)
        return (resultSign << 31) | POS_INF;

    // Zero / finite
    if (xZero)
        return resultSign << 31;

    // Normalize subnormal significands and calculate effective exponents.
    uint32_t sigA = x.significand;
    uint32_t sigB = y.significand;

    int32_t expA;
    int32_t expB;

    if (x.exponent == 0)
    {
        expA = -126;

        while (sigA < (1u << 23))
        {
            sigA <<= 1;
            expA--;
        }
    }
    else
    {
        expA = static_cast<int32_t>(x.exponent) - 127;
    }

    if (y.exponent == 0)
    {
        expB = -126;

        while (sigB < (1u << 23))
        {
            sigB <<= 1;
            expB--;
        }
    }
    else
    {
        expB = static_cast<int32_t>(y.exponent) - 127;
    }

    // Encode both normalized significands as binary32 values in [1, 2).
    uint32_t normA = makeNormalizedFloat(sigA);
    uint32_t normB = makeNormalizedFloat(sigB);

    // Goldschmidt: approximate 1 / normB.
    uint32_t reciprocalB = goldschmidtReciprocal(normB);

    // Divide the significands by multiplying by the reciprocal.
    uint32_t product = fmul(normA, reciprocalB);

    // Apply the exponent difference.
    int32_t exponentField =
        static_cast<int32_t>((product >> 23) & 0xFFu) + expA - expB;

    uint32_t fraction = product & 0x7FFFFFu;

    // Overflow to infinity.
    if (exponentField >= 255)
        return (resultSign << 31) | POS_INF;

    // Underflow: round the significand into the subnormal range.
    if (exponentField <= 0)
    {
        uint64_t significand = (1u << 23) | fraction;
        int32_t shift = 1 - exponentField;

        if (shift >= 64)
            return resultSign << 31;

        uint64_t rounded;

        if (shift == 0)
        {
            rounded = significand;
        }
        else
        {
            uint64_t quotient = significand >> shift;
            uint64_t remainder =
                significand & ((1ULL << shift) - 1ULL);
            uint64_t halfway = 1ULL << (shift - 1);

            if (remainder > halfway ||
                (remainder == halfway && (quotient & 1ULL)))
            {
                quotient++;
            }

            rounded = quotient;
        }

        // Rounding may promote a subnormal to the smallest normal.
        if (rounded >= (1ULL << 23))
            return (resultSign << 31) | (1u << 23);

        return (resultSign << 31) |
               static_cast<uint32_t>(rounded);
    }

    // Pack the normal result.
    return (resultSign << 31) |
           (static_cast<uint32_t>(exponentField) << 23) |
           fraction;
}

// Square root implementation

uint32_t FPU::sqrtInitialGuess(uint32_t a)
{
    // Initial estimate for a positive, finite binary32 input.
    return (a >> 1) + 0x1FC00000u;
}

uint32_t FPU::sqrt(uint32_t a)
{
    FloatParts x = unpack(a);

    const uint32_t QUIET_NAN = 0x7FC00000u;
    const uint32_t POS_INF = 0x7F800000u;
    const uint32_t HALF = 0x3F000000u;

    bool isNaN = (x.exponent == 255 && x.fraction != 0);
    bool isInf = (x.exponent == 255);
    bool isZero = ((a & 0x7FFFFFFFu) == 0);
    bool isNegative = ((a >> 31) != 0);

    // NaN: return a quiet NaN.
    if (isNaN)
        return QUIET_NAN;

    // Negative nonzero input: invalid result.
    if (isNegative && !isZero)
        return QUIET_NAN;

    // Preserve positive and negative zero.
    if (isZero)
        return a;

    // Positive infinity.
    if (isInf)
        return POS_INF;

    // Initial estimate.
    // Scale subnormal inputs into the normal range.
    bool wasSubnormal = (x.exponent == 0);

    uint32_t input = a;

    if (wasSubnormal)
    {
        input = fmul(a, 0x4B800000u); // Multiply by 2^24
    }

    uint32_t y = sqrtInitialGuess(input);

    // Newton-Raphson iterations.
    for (int i = 0; i < 5; i++)
    {
        uint32_t quotient = fdiv(input, y);
        uint32_t sum = fadd(y, quotient);
        y = fmul(0x3F000000u, sum);
    }

    // sqrt(a * 2^24) = sqrt(a) * 2^12.
    // Therefore, divide the result by 2^12 to undo the scaling.
    if (wasSubnormal)
    {
        y = fmul(y, 0x39800000u); // Multiply by 2^-12
    }

    return y;
}

// sin implemetnation
uint32_t FPU::sin(uint32_t a)
{
    const uint32_t QUIET_NAN = 0x7FC00000u;
    const uint32_t PI = 0x40490FDBu;
    const uint32_t HALF_PI = 0x3FC90FDBu;
    const uint32_t TWO_PI = 0x40C90FDBu;
    const uint32_t ONE = 0x3F800000u;

    // Taylor coefficients.
    const uint32_t C1 = 0xBE2AAAABu; // -1/6
    const uint32_t C2 = 0x3C088889u; //  1/120
    const uint32_t C3 = 0xB9500D01u; // -1/5040
    const uint32_t C4 = 0x3638EF1Du; //  1/362880

    uint32_t exponent = (a >> 23) & 0xFFu;

    // NaN or infinity.
    if (exponent == 255u)
        return QUIET_NAN;

    // Preserve positive and negative zero.
    if ((a & 0x7FFFFFFFu) == 0)
        return a;

    // Extract sign and magnitude.
    bool negative = (a >> 31) != 0;
    uint32_t x = a & 0x7FFFFFFFu;

    // Range reduction: intended for moderate-sized angles.
    int iterations = 0;

    while (x >= TWO_PI && iterations < 1000)
    {
        x = fsub(x, TWO_PI);
        iterations++;
    }

    // If x is in (pi, 2*pi), sine is negative.
    if (x > PI)
    {
        x = fsub(x, PI);
        negative = !negative;
    }

    // Reflect into [0, pi/2].
    if (x > HALF_PI)
    {
        x = fsub(PI, x);
    }

    // Horner evaluation:
    // sin(x) ~= x * (1 - x^2/6 + x^4/120
    //                    - x^6/5040 + x^8/362880)

    uint32_t x2 = fmul(x, x);

    uint32_t p = C4;
    p = fadd(C3, fmul(x2, p));
    p = fadd(C2, fmul(x2, p));
    p = fadd(C1, fmul(x2, p));
    p = fadd(ONE, fmul(x2, p));

    uint32_t result = fmul(x, p);

    // Restore the correct sign.
    if (negative)
        result ^= 0x80000000u;

    return result;
}

// cos implementation
uint32_t FPU::cos(uint32_t a)
{
    const uint32_t QUIET_NAN = 0x7FC00000u;
    const uint32_t PI = 0x40490FDBu;
    const uint32_t HALF_PI = 0x3FC90FDBu;
    const uint32_t TWO_PI = 0x40C90FDBu;
    const uint32_t ONE = 0x3F800000u;

    // Taylor coefficients:
    // cos(x) ~= 1 - x^2/2 + x^4/24 - x^6/720 + x^8/40320
    const uint32_t C1 = 0xBF000000u; // -1/2
    const uint32_t C2 = 0x3D2AAAABu; //  1/24
    const uint32_t C3 = 0xBAB60B61u; // -1/720
    const uint32_t C4 = 0x37D00D01u; //  1/40320

    uint32_t exponent = (a >> 23) & 0xFFu;

    // NaN or infinity.
    if (exponent == 255u)
        return QUIET_NAN;

    // cos(+0) = cos(-0) = 1.
    if ((a & 0x7FFFFFFFu) == 0)
        return ONE;

    // Extract sign and magnitude.
    uint32_t x = a & 0x7FFFFFFFu;

    // Range reduction for moderate-sized angles.
    int iterations = 0;

    while (x >= TWO_PI && iterations < 1000)
    {
        x = fsub(x, TWO_PI);
        iterations++;
    }

    bool negative = false;

    // cos(x) is negative in the second and third quadrants.
    if (x > PI)
    {
        x = fsub(x, PI);
        negative = true;
    }

    // Reflect into [0, pi/2].
    if (x > HALF_PI)
    {
        x = fsub(PI, x);
        negative = !negative;
    }

    uint32_t x2 = fmul(x, x);

    uint32_t p = C4;
    p = fadd(C3, fmul(x2, p));
    p = fadd(C2, fmul(x2, p));
    p = fadd(C1, fmul(x2, p));
    p = fadd(ONE, fmul(x2, p));

    uint32_t result = p;

    if (negative)
        result ^= 0x80000000u;

    return result;
}

//cos inverse implementation 
uint32_t FPU::acos(uint32_t a)
{
    const uint32_t QUIET_NAN = 0x7FC00000u;
    const uint32_t ONE       = 0x3F800000u;
    const uint32_t PI        = 0x40490FDBu;
    const uint32_t C0        = 0x3FC90F80u; //  1.5707288
    const uint32_t C1        = 0xBE5954FDu; // -0.2121144
    const uint32_t C2        = 0x3D97F62Bu; //  0.0742610
    const uint32_t C3        = 0xBC996B9Au; // -0.0187293

    uint32_t exponent = (a >> 23) & 0xFFu;

    // NaN input
    if (exponent == 255u && (a & 0x7FFFFFu) != 0)
        return QUIET_NAN;

    // Input outside [-1, 1]
    if ((a & 0x7FFFFFFFu) > ONE)
        return QUIET_NAN;

    // t = |x|
    uint32_t t = fabs(a);

    // p(t) = ((C3*t + C2)*t + C1)*t + C0
    uint32_t p = fadd(fmul(C3, t), C2);
    p = fadd(fmul(p, t), C1);
    p = fadd(fmul(p, t), C0);

    // r = sqrt(1 - t) * p(t)
    uint32_t oneMinusT = fsub(ONE, t);
    uint32_t r = fmul(sqrt(oneMinusT), p);

    // acos(x) = r if x >= 0, otherwise pi - r
    if ((a >> 31) != 0)
        return fsub(PI, r);

    return r;
}