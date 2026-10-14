#ifndef BITFORGE_TEST_HARNESS_HPP
#define BITFORGE_TEST_HARNESS_HPP

#include "bitforge/core/types.hpp"
#include "bitforge/core/constants.hpp"
#include "bitforge/core/cpu_state.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <type_traits>
#include <functional>
#include <iomanip>

namespace bitforge::test {

// Detection idiom for types supporting std::ostream <<
template <typename T, typename = void>
struct is_streamable : std::false_type {};

template <typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<const T&>())>> : std::true_type {};

// Robust diagnostic printer handling integer promotions and non-streamable types
template <typename Stream, typename T>
void print_val(Stream& os, const T& val) {
    if constexpr (std::is_same_v<std::decay_t<T>, byte_t> ||
                  std::is_same_v<std::decay_t<T>, sbyte_t> ||
                  std::is_same_v<std::decay_t<T>, char> ||
                  std::is_same_v<std::decay_t<T>, unsigned char> ||
                  std::is_same_v<std::decay_t<T>, signed char>) {
        os << static_cast<int>(val);
    } else if constexpr (is_streamable<T>::value) {
        os << val;
    } else {
        os << "<object>";
    }
}

// Global test execution context
struct TestContext {
    int current_test_failures{0};
    int total_assertions{0};
    int total_failures{0};
    int tests_passed{0};
    int tests_failed{0};

    static TestContext& instance() {
        static TestContext ctx;
        return ctx;
    }
};

struct TestCase {
    std::string suite;
    std::string name;
    void (*func)();
};

inline std::vector<TestCase>& get_test_registry() {
    static std::vector<TestCase> registry;
    return registry;
}

inline bool register_test(const char* suite, const char* name, void (*func)()) {
    get_test_registry().push_back(TestCase{suite, name, func});
    return true;
}

inline void record_failure(const char* file, int line, const std::string& message) {
    TestContext::instance().current_test_failures++;
    TestContext::instance().total_failures++;
    std::cerr << "  FAILURE at " << file << ":" << line << "\n"
              << "    " << message << "\n";
}

inline int run_all_tests() {
    auto& registry = get_test_registry();
    auto& ctx = TestContext::instance();

    std::cout << "====================================================\n";
    std::cout << "Running " << registry.size() << " BitForge test cases...\n";
    std::cout << "====================================================\n";

    for (const auto& test : registry) {
        ctx.current_test_failures = 0;
        std::cout << "[ RUN      ] " << test.suite << "." << test.name << "\n";
        
        try {
            test.func();
        } catch (const std::exception& ex) {
            record_failure(__FILE__, __LINE__, std::string("Unexpected exception: ") + ex.what());
        } catch (...) {
            record_failure(__FILE__, __LINE__, "Unknown non-standard exception caught");
        }

        if (ctx.current_test_failures == 0) {
            ctx.tests_passed++;
            std::cout << "[       OK ] " << test.suite << "." << test.name << "\n";
        } else {
            ctx.tests_failed++;
            std::cout << "[  FAILED  ] " << test.suite << "." << test.name 
                      << " (" << ctx.current_test_failures << " failures)\n";
        }
    }

    std::cout << "====================================================\n";
    std::cout << "Test Summary: " << ctx.tests_passed << " passed, " 
              << ctx.tests_failed << " failed out of " << registry.size() << " total tests.\n";
    std::cout << "====================================================\n";

    return (ctx.tests_failed == 0) ? 0 : 1;
}

// -------------------------------------------------------------
// Section 12.1 Instruction Budget and CPU Runner Template
// -------------------------------------------------------------
template <typename CpuType>
struct RunResult {
    bool success{false};
    std::uint64_t steps_executed{0};
    std::string stop_reason;
};

template <typename CpuType>
RunResult<CpuType> run_with_budget(CpuType& cpu, std::uint64_t max_instructions = kDefaultInstructionBudget) {
    RunResult<CpuType> res;
    while ((cpu.state() == MachineState::Ready || cpu.state() == MachineState::Running) &&
           res.steps_executed < max_instructions) {
        auto step_res = cpu.step();
        ++res.steps_executed;
        if (step_res.faulted || cpu.state() == MachineState::Faulted) {
            res.stop_reason = "Faulted";
            res.success = false;
            return res;
        }
        if (step_res.halted || cpu.state() == MachineState::Halted) {
            res.stop_reason = "Halted";
            res.success = true;
            return res;
        }
    }

    if (res.steps_executed >= max_instructions && cpu.state() != MachineState::Halted) {
        res.stop_reason = "InstructionBudgetExceeded";
        res.success = false;
        std::cerr << "TEST FAILURE: Instruction budget exceeded (" << max_instructions << " steps)\n"
                  << "  Machine State: " << to_string(cpu.state()) << "\n"
                  << "  Final PC: 0x" << std::hex << cpu.registers().pc << "\n"
                  << "  Opcode at PC: 0x" << static_cast<int>(cpu.bus().read_byte(cpu.registers().pc)) << "\n"
                  << "  R0: 0x" << static_cast<int>(cpu.registers().r[0])
                  << " R1: 0x" << static_cast<int>(cpu.registers().r[1])
                  << " R2: 0x" << static_cast<int>(cpu.registers().r[2])
                  << " R3: 0x" << static_cast<int>(cpu.registers().r[3]) << "\n"
                  << "  IX: 0x" << cpu.registers().ix << " SP: 0x" << cpu.registers().sp << "\n"
                  << "  Flags: [Z=" << cpu.flags().z << " C=" << cpu.flags().c
                  << " N=" << cpu.flags().n << " V=" << cpu.flags().v << "]\n" << std::dec;
    }
    return res;
}

} // namespace bitforge::test

// -------------------------------------------------------------
// Test Declaration and Assertion Macros
// -------------------------------------------------------------
#define BITFORGE_TEST_CONCAT_INNER(a, b) a##b
#define BITFORGE_TEST_CONCAT(a, b) BITFORGE_TEST_CONCAT_INNER(a, b)

#define TEST(suite, name) \
    void test_##suite##_##name(); \
    namespace { \
        const bool BITFORGE_TEST_CONCAT(registered_, __LINE__) = \
            ::bitforge::test::register_test(#suite, #name, &test_##suite##_##name); \
    } \
    void test_##suite##_##name()

#define EXPECT_TRUE(expr) \
    do { \
        ::bitforge::test::TestContext::instance().total_assertions++; \
        if (!(expr)) { \
            ::bitforge::test::record_failure(__FILE__, __LINE__, "Expected true: " #expr); \
        } \
    } while (0)

#define EXPECT_FALSE(expr) \
    do { \
        ::bitforge::test::TestContext::instance().total_assertions++; \
        if (expr) { \
            ::bitforge::test::record_failure(__FILE__, __LINE__, "Expected false: " #expr); \
        } \
    } while (0)

#define EXPECT_EQ(val1, val2) \
    do { \
        ::bitforge::test::TestContext::instance().total_assertions++; \
        const auto v1 = (val1); \
        const auto v2 = (val2); \
        if (!(v1 == v2)) { \
            std::ostringstream oss; \
            oss << "Expected equality: " #val1 " == " #val2 "\n" \
                << "      Actual: "; \
            ::bitforge::test::print_val(oss, v1); \
            oss << " vs "; \
            ::bitforge::test::print_val(oss, v2); \
            ::bitforge::test::record_failure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_NE(val1, val2) \
    do { \
        ::bitforge::test::TestContext::instance().total_assertions++; \
        const auto v1 = (val1); \
        const auto v2 = (val2); \
        if (v1 == v2) { \
            std::ostringstream oss; \
            oss << "Expected inequality: " #val1 " != " #val2 "\n" \
                << "      Actual: "; \
            ::bitforge::test::print_val(oss, v1); \
            oss << " == "; \
            ::bitforge::test::print_val(oss, v2); \
            ::bitforge::test::record_failure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_LT(val1, val2) \
    do { \
        ::bitforge::test::TestContext::instance().total_assertions++; \
        const auto v1 = (val1); \
        const auto v2 = (val2); \
        if (!(v1 < v2)) { \
            std::ostringstream oss; \
            oss << "Expected: " #val1 " < " #val2 "\n" \
                << "      Actual: "; \
            ::bitforge::test::print_val(oss, v1); \
            oss << " >= "; \
            ::bitforge::test::print_val(oss, v2); \
            ::bitforge::test::record_failure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_LE(val1, val2) \
    do { \
        ::bitforge::test::TestContext::instance().total_assertions++; \
        const auto v1 = (val1); \
        const auto v2 = (val2); \
        if (!(v1 <= v2)) { \
            std::ostringstream oss; \
            oss << "Expected: " #val1 " <= " #val2 "\n" \
                << "      Actual: "; \
            ::bitforge::test::print_val(oss, v1); \
            oss << " > "; \
            ::bitforge::test::print_val(oss, v2); \
            ::bitforge::test::record_failure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_GT(val1, val2) \
    do { \
        ::bitforge::test::TestContext::instance().total_assertions++; \
        const auto v1 = (val1); \
        const auto v2 = (val2); \
        if (!(v1 > v2)) { \
            std::ostringstream oss; \
            oss << "Expected: " #val1 " > " #val2 "\n" \
                << "      Actual: "; \
            ::bitforge::test::print_val(oss, v1); \
            oss << " <= "; \
            ::bitforge::test::print_val(oss, v2); \
            ::bitforge::test::record_failure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_GE(val1, val2) \
    do { \
        ::bitforge::test::TestContext::instance().total_assertions++; \
        const auto v1 = (val1); \
        const auto v2 = (val2); \
        if (!(v1 >= v2)) { \
            std::ostringstream oss; \
            oss << "Expected: " #val1 " >= " #val2 "\n" \
                << "      Actual: "; \
            ::bitforge::test::print_val(oss, v1); \
            oss << " < "; \
            ::bitforge::test::print_val(oss, v2); \
            ::bitforge::test::record_failure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define ASSERT_TRUE(expr) \
    do { \
        EXPECT_TRUE(expr); \
        if (!(expr)) return; \
    } while (0)

#define ASSERT_EQ(val1, val2) \
    do { \
        EXPECT_EQ(val1, val2); \
        if (!((val1) == (val2))) return; \
    } while (0)

#endif // BITFORGE_TEST_HARNESS_HPP
