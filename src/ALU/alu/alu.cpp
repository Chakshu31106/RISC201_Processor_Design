#include "alu.h"

ALU::ALU()
{
    E = false;
    GT = false;
    ST = false;
}

// Logical
// Basic logical operations used as primitives for ALU implementation.
int32_t ALU::AND(int32_t a, int32_t b)
{
    return a & b;
}
int32_t ALU::OR(int32_t a, int32_t b)
{
    return a | b;
}
int32_t ALU::NOT(int32_t a)
{
    return ~a;
}
int32_t ALU::NOR(int32_t a, int32_t b)
{
    return ~(a | b);
}
int32_t ALU::NAND(int32_t a, int32_t b)
{
    return ~(a & b);
}
int32_t ALU::XOR(int32_t a, int32_t b)
{
    return a ^ b;
}


//ARITHMETIC

// CARRY LOOKAHEAD ADDER FOR FASTER ADDITION

void ALU::eightBitCLA(int32_t a, int32_t b, int carryIn, int32_t &sum, int &carryOut, int &groupPropagate, int &groupGenerate)
{
    int p[8];
    int g[8];
    int c[9];

    c[0] = carryIn;

    // Calculate propagate and generate
    for (int i = 0; i < 8; i++)
    {
        int ai = (a >> i) & 1;
        int bi = (b >> i) & 1;

        p[i] = XOR(ai, bi);
        g[i] = AND(ai, bi);
    }

    // Carry lookahead equations
    // Calculate carry lookahead
    for (int i = 0; i < 8; i++)
    {
        int carry = g[i];
        int propagate = 1;

        for (int j = i; j >= 0; j--)
        {
            propagate = AND(propagate, p[j]);

            int term;

            if (j == 0)
                term = AND(propagate, c[0]);
            else
                term = AND(propagate, g[j - 1]);

            carry = OR(carry, term);
        }

        c[i + 1] = carry;
    }
    // Group propagate for 8 bit block
    groupPropagate = 1;

    for (int i = 0; i < 8; i++)
    {
        groupPropagate = AND(groupPropagate, p[i]);
    }
    // Group Generate for 8 bit block
    groupGenerate = g[7];

    int propagate = p[7];

    for (int i = 6; i >= 0; i--)
    {
        groupGenerate = OR(
            groupGenerate,
            AND(propagate, g[i]));

        propagate = AND(propagate, p[i]);
    }
    // Calculate sum bits
    sum = 0;

    for (int i = 0; i < 8; i++)
    {
        int s = XOR(p[i], c[i]);
        sum |= (s << i);
    }

    carryOut = c[8];
}

void ALU::blockCarryLookahead(
    int pg[4],
    int gg[4],
    int carryIn,
    int carry[5]
)
{
    carry[0] = carryIn;

    for (int i = 0; i < 4; i++)
    {
        int blockCarry = gg[i];
        int propagate = 1;

        for (int j = i; j >= 0; j--)
        {
            propagate = AND(propagate, pg[j]);

            int term;

            if (j == 0)
                term = AND(propagate, carryIn);
            else
                term = AND(propagate, gg[j - 1]);

            blockCarry = OR(blockCarry, term);
        }

        carry[i + 1] = blockCarry;
    }
}

int32_t ALU::add(int32_t a, int32_t b)
{
    int32_t sum[4];

    int pg[4];
    int gg[4];

    int blockCarry[5];

    // Extract the four 8-bit blocks
    int32_t aBlock[4];
    int32_t bBlock[4];

    for (int i = 0; i < 4; i++)
    {
        aBlock[i] = (a >> (8 * i)) & 0xFF;
        bBlock[i] = (b >> (8 * i)) & 0xFF;
    }

    // First calculate P/G information for each block.
    // We use a temporary carry-in of 0 here.
    for (int i = 0; i < 4; i++)
    {
        int dummyCarry;

        eightBitCLA(
            aBlock[i],
            bBlock[i],
            0,
            sum[i],
            dummyCarry,
            pg[i],
            gg[i]
        );
    }

    // Calculate carry between the four blocks.
    blockCarryLookahead(
        pg,
        gg,
        0,
        blockCarry
    );

    // Recalculate each block with its actual carry-in.
    for (int i = 0; i < 4; i++)
    {
        int dummyCarry;
        int dummyPG;
        int dummyGG;

        eightBitCLA(
            aBlock[i],
            bBlock[i],
            blockCarry[i],
            sum[i],
            dummyCarry,
            dummyPG,
            dummyGG
        );
    }

    // Combine the four 8-bit results.
    int32_t result = 0;

    for (int i = 0; i < 4; i++)
    {
        result |= (sum[i] & 0xFF) << (8 * i);
    }

    return result;
}

// SUBTRACTION USING CLA AND COMPLEMENT

int32_t ALU::sub(int32_t a, int32_t b)
{
    int32_t sum[4];

    int pg[4];
    int gg[4];

    int blockCarry[5];

    // Extract the four 8-bit blocks
    int32_t aBlock[4];
    int32_t bBlock[4];

    for (int i = 0; i < 4; i++)
    {
        aBlock[i] = (a >> (8 * i)) & 0xFF;
        bBlock[i] = (b >> (8 * i)) & 0xFF;

        // Two's complement subtraction:
        // A - B = A + (~B) + 1
        bBlock[i] = (~bBlock[i]) & 0xFF;
    }

    // Calculate P/G information for each block.
    // Carry-in is 0 here because we only need P/G.
    for (int i = 0; i < 4; i++)
    {
        int dummyCarry;

        eightBitCLA(
            aBlock[i],
            bBlock[i],
            0,
            sum[i],
            dummyCarry,
            pg[i],
            gg[i]
        );
    }

    // For subtraction, the initial carry-in is 1.
    blockCarryLookahead(
        pg,
        gg,
        1,
        blockCarry
    );

    // Calculate each block using its actual carry-in.
    for (int i = 0; i < 4; i++)
    {
        int dummyCarry;
        int dummyPG;
        int dummyGG;

        eightBitCLA(
            aBlock[i],
            bBlock[i],
            blockCarry[i],
            sum[i],
            dummyCarry,
            dummyPG,
            dummyGG
        );
    }

    // Combine the four 8-bit results.
    int32_t result = 0;

    for (int i = 0; i < 4; i++)
    {
        result |= (sum[i] & 0xFF) << (8 * i);
    }

    return result;
}
       
//  MULTIPLICATION USING BOOTH'S ALGO

void ALU::boothShift(
    int32_t &A,
    uint32_t &Q,
    int &Qminus1)
{
    // Save the bits that will move during the shift.
    int oldA0 = A & 1;
    int oldQ0 = Q & 1;

    // Arithmetic right shift A.
    A = A >> 1;

    // Logical right shift Q.
    Q = Q >> 1;

    // Old A0 becomes the new MSB of Q.
    Q = Q | (static_cast<uint32_t>(oldA0) << 31);

    // Old Q0 becomes Q-1.
    Qminus1 = oldQ0;
}

int32_t ALU::mul(int32_t a, int32_t b)
{
    int32_t A = 0;
    int32_t M = a;

    uint32_t Q = static_cast<uint32_t>(b);

    int Qminus1 = 0;

    for (int i = 0; i < 32; i++)
    {
        int Q0 = Q & 1;

        // Booth decision
        if (Q0 == 0 && Qminus1 == 1)
        {
            // 01 -> A = A + M
            A = add(A, M);
        }
        else if (Q0 == 1 && Qminus1 == 0)
        {
            // 10 -> A = A - M
            A = sub(A, M);
        }

        // Arithmetic right shift of A:Q:Q-1
        boothShift(A, Q, Qminus1);
    }

    // Lower 32 bits of the product.
    return static_cast<int32_t>(Q);
}

// DIVISION

void ALU::restoringDivide(
    uint32_t dividend,
    uint32_t divisor,
    uint32_t &quotient,
    uint32_t &remainder)
{
    uint64_t R = 0;
    uint32_t Q = dividend;

    quotient = 0;

    for (int i = 31; i >= 0; i--)
    {
        // Shift R left and bring in the next dividend bit.
        R = (R << 1) | ((Q >> i) & 1);

        // Try subtracting the divisor.
        if (R >= divisor)
        {
            R = R - divisor;

            // Set the corresponding quotient bit.
            quotient |= (1u << i);
        }
    }

    remainder = static_cast<uint32_t>(R);
}

int32_t ALU::div(int32_t a, int32_t b)
{
    // Division by zero
    if (b == 0)
        return 0;   // temporary convention; document/change if ISA specifies otherwise

    bool negative = (a < 0) ^ (b < 0);

    uint32_t ua = (a < 0)
        ? static_cast<uint32_t>(-(int64_t)a)
        : static_cast<uint32_t>(a);

    uint32_t ub = (b < 0)
        ? static_cast<uint32_t>(-(int64_t)b)
        : static_cast<uint32_t>(b);

    uint32_t quotient;
    uint32_t remainder;

    restoringDivide(ua, ub, quotient, remainder);

    if (negative)
    {
        quotient = ~quotient + 1;
    }

    return static_cast<int32_t>(quotient);
}

// MODULAR

int32_t ALU::mod(int32_t a, int32_t b)
{
    // Division by zero
    if (b == 0)
        return 0;   // temporary convention

    // Remainder sign follows the dividend.
    bool negative = (a < 0);

    // Get magnitudes safely.
    uint32_t ua = (a < 0)
        ? static_cast<uint32_t>(-(int64_t)a)
        : static_cast<uint32_t>(a);

    uint32_t ub = (b < 0)
        ? static_cast<uint32_t>(-(int64_t)b)
        : static_cast<uint32_t>(b);

    uint32_t quotient;
    uint32_t remainder;

    restoringDivide(ua, ub, quotient, remainder);

    // C/C++-style remainder: sign follows dividend.
    if (negative)
    {
        remainder = ~remainder + 1;
    }

    return static_cast<int32_t>(remainder);
}

// COMPARING LOGIC

int32_t ALU::cmp(int32_t a, int32_t b)
{
    if (a == b)
    {
        E = true;
        GT = false;
        ST = false;
    }
    else if (a > b)
    {
        E = false;
        GT = true;
        ST = false;
    }
    else
    {
        E = false;
        GT = false;
        ST = true;
    }

    return 0;
}

bool ALU::getE()
{
    return E;
}

bool ALU::getGT()
{
    return GT;
}

bool ALU::getST()
{
    return ST;
}

// INCREASING AND DECREASING AND POW AND ABSOLUTE 

int32_t ALU::incr(int32_t a)
{
    return add(a, 1);
}

int32_t ALU::decr(int32_t a)
{
    return sub(a,1);
}

int32_t ALU::pow(int32_t a, int32_t b)
{
    if (b < 0)
        return 0;  // negative exponent not supported for integer result

    int32_t result = 1;

    while (b > 0)
    {
        if (b & 1)
            result = mul(result, a);

        a = mul(a, a);
        b = b >> 1;
    }

    return result;
}

int32_t ALU::abs(int32_t a)
{
    if (a < 0)
        return sub(0, a);

    return a;
}


// SHIFT OPERATION 

int32_t ALU::lsl(int32_t a, int32_t n)
{
    if (n < 0 || n >= 32)
        return 0;

    return a << n;
}

int32_t ALU::lsr(int32_t a, int32_t n)
{
    if (n < 0 || n >= 32)
        return 0;

    uint32_t value = static_cast<uint32_t>(a);
    return static_cast<int32_t>(value >> n);
}

int32_t ALU::asr(int32_t a, int32_t n)
{
    if (n < 0 || n >= 32)
        return 0;

    return a >> n;
}

int32_t ALU::rotl(int32_t a, int32_t n)
{
    if (n < 0 || n >= 32)
        return 0;

    if (n == 0)
        return a;

    uint32_t value = static_cast<uint32_t>(a);

    return static_cast<int32_t>(
        (value << n) | (value >> (32 - n))
    );
}

int32_t ALU::rotr(int32_t a, int32_t n)
{
    if (n < 0 || n >= 32)
        return 0;

    if (n == 0)
        return a;

    uint32_t value = static_cast<uint32_t>(a);

    return static_cast<int32_t>(
        (value >> n) | (value << (32 - n))
    );
}

// BIT MANIPULATION

int32_t ALU::pcnt(int32_t a)
{
    uint32_t value = static_cast<uint32_t>(a);
    int32_t count = 0;

    for (int i = 0; i < 32; i++)
    {
        count += value & 1;
        value = value >> 1;
    }

    return count;
}

int32_t ALU::tog(int32_t a, int32_t n)
{
    if (n < 0 || n >= 32)
        return a;

    uint32_t value = static_cast<uint32_t>(a);
    value = value ^ (1u << n);

    return static_cast<int32_t>(value);
}

int32_t ALU::sbit(int32_t a, int32_t n)
{
    if (n < 0 || n >= 32)
        return a;

    uint32_t value = static_cast<uint32_t>(a);
    value = value | (1u << n);

    return static_cast<int32_t>(value);
}

int32_t ALU::cbit(int32_t a, int32_t n)
{
    if (n < 0 || n >= 32)
        return a;

    uint32_t value = static_cast<uint32_t>(a);
    value = value & ~(1u << n);

    return static_cast<int32_t>(value);
}

