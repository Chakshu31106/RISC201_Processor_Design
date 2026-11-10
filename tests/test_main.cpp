#include <cstring>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include "datapath/register_file.h"
#include "test_framework.h"

namespace testing {

namespace {
std::vector<std::pair<const char*, TestFn>>& registry() {
    static std::vector<std::pair<const char*, TestFn>> r;
    return r;
}
int g_checks = 0;
int g_failures = 0;
}  // namespace

Registrar::Registrar(const char* name, TestFn fn) { registry().emplace_back(name, fn); }

void check(bool ok, const char* expr, const char* file, int line) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::cerr << "  FAIL " << file << ":" << line << "  " << expr << "\n";
    }
}

}  // namespace testing

// Usage: risc201_tests [substring]   -- runs only tests whose name contains it.
int main(int argc, char** argv) {
    using namespace testing;
    const char* filter = argc > 1 ? argv[1] : nullptr;
    risc201::display_format() = risc201::NumFormat::Int;   // the older tests expect plain integer output

    int run = 0, failed_tests = 0;
    for (auto& [name, fn] : registry()) {
        if (filter && !std::strstr(name, filter)) continue;
        ++run;
        const int before = g_failures;
        try {
            fn();
        } catch (const std::exception& e) {
            ++g_failures;
            std::cerr << "  FAIL " << name << ": uncaught exception: " << e.what() << "\n";
        }
        const bool ok = (g_failures == before);
        if (!ok) ++failed_tests;
        std::cout << (ok ? "[ OK ] " : "[FAIL] ") << name << "\n";
    }

    std::cout << "\n" << (run - failed_tests) << "/" << run << " tests passed ("
              << (g_checks - g_failures) << "/" << g_checks << " checks)\n";
    return g_failures == 0 ? 0 : 1;
}
