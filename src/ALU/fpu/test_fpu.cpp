#include <iostream>
#include <cstdint>
#include <cstring>
#include <cmath>
#include "fpu.h"

using namespace std;

uint32_t floatToBits(float x)
{
    uint32_t bits;
    memcpy(&bits, &x, sizeof(bits));
    return bits;
}

float bitsToFloat(uint32_t bits)
{
    float x;
    memcpy(&x, &bits, sizeof(x));
    return x;
}

void testAcos(FPU &fpu)
{
    cout << "\n--- Testing ACOS ---\n";

    struct TestCase {
        float input;
        float expected;
        const char* name;
    };

    TestCase tests[] = {
        { 1.0f,   0.0f,                "acos(1)" },
        {-1.0f,   3.14159265f,         "acos(-1)" },
        { 0.0f,   1.57079633f,         "acos(0)" },
        { 0.5f,   1.04719755f,         "acos(0.5)" },
        {-0.5f,   2.09439510f,         "acos(-0.5)" },
        { 0.25f,  1.31811607f,         "acos(0.25)" },
        {-0.25f,  1.82347658f,         "acos(-0.25)" },
        { 0.9f,   0.45102681f,         "acos(0.9)" },
        {-0.9f,   2.69056584f,         "acos(-0.9)" },
        { 0.99f,  0.14153947f,         "acos(0.99)" },
        {-0.99f,  3.00005318f,         "acos(-0.99)" },
        { 0.9999f, 0.01414225f, "acos(0.9999)" },
{-0.9999f, 3.1274504f,  "acos(-0.9999)"}
    };

    const float tolerance = 0.002f;
    int passed = 0;
    int total = sizeof(tests) / sizeof(tests[0]);

    for (const auto &tc : tests) {
        float result = bitsToFloat(fpu.acos(floatToBits(tc.input)));
        float error = fabsf(result - tc.expected);

        bool ok = error <= tolerance;

        cout << (ok ? "[PASS] " : "[FAIL] ")
             << tc.name
             << " | Expected: " << tc.expected
             << " | Got: " << result
             << " | Error: " << error << '\n';

        if (ok) passed++;
    }

    // Invalid inputs should return NaN.
    float invalidInputs[] = {1.1f, -1.1f, 2.0f, -2.0f};

    for (float input : invalidInputs) {
        float result = bitsToFloat(fpu.acos(floatToBits(input)));
        bool ok = std::isnan(result);

        cout << (ok ? "[PASS] " : "[FAIL] ")
             << "acos(" << input << ") invalid input"
             << " | Got: " << result << '\n';

        if (ok) passed++;
        total++;
    }

    cout << "ACOS Results: " << passed << "/" << total
         << " tests passed\n";
}

int main()
{
    FPU fpu;
    testAcos(fpu);
    return 0;
}
