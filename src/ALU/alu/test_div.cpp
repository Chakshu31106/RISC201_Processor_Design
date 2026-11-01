#include <iostream>
#include <cstdint>
#include "alu.h"
#include <random>

using namespace std;

int main()
{
    ALU alu;

    mt19937 rng(12345);

for (int i = 0; i < 100000; i++)
{
    int32_t a = static_cast<int32_t>(rng());
    int32_t b = static_cast<int32_t>(rng());

    if (b == 0)
        b = 1;

    if (a == INT32_MIN && b == -1)
        b = 1;

    if (alu.div(a, b) != a / b ||
        alu.mod(a, b) != a % b)
    {
        cout << "FAILED!" << endl;
        cout << "a = " << a << ", b = " << b << endl;
        cout << "Your div = " << alu.div(a, b)
             << ", expected = " << a / b << endl;
        cout << "Your mod = " << alu.mod(a, b)
             << ", expected = " << a % b << endl;
        return 1;
    }
}

cout << "All 100000 random tests passed!" << endl;
    return 0;
}