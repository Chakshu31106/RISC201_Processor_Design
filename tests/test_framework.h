#pragma once

// Minimal self-registering test framework (no external dependencies).
//
//   TEST(name) { CHECK(cond); CHECK_EQ(a, b); }
//
// Tests register themselves at static-init time; test_main.cpp runs them.

#include <iostream>
#include <string>

namespace testing {

using TestFn = void (*)();

struct Registrar {
    Registrar(const char* name, TestFn fn);
};

void check(bool ok, const char* expr, const char* file, int line);

template <class A, class B>
void check_eq(const A& a, const B& b, const char* ea, const char* eb,
              const char* file, int line) {
    const bool ok = (a == b);
    check(ok, (std::string(ea) + " == " + eb).c_str(), file, line);
    if (!ok) std::cerr << "      left:  " << a << "\n      right: " << b << "\n";
}

}  // namespace testing

#define TEST(name)                                                       \
    static void test_##name();                                           \
    static ::testing::Registrar registrar_##name(#name, test_##name);    \
    static void test_##name()

#define CHECK(cond) ::testing::check((cond), #cond, __FILE__, __LINE__)
#define CHECK_EQ(a, b) ::testing::check_eq((a), (b), #a, #b, __FILE__, __LINE__)
