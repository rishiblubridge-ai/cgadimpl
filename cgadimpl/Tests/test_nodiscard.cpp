//==========================
//cgadimpl/Tests/test_nodiscard.cpp
//===========================


#include "ad/core/nodiscard.hpp"

#include <vector>
#include <string>
#include <memory>
#include <utility>

// Helper to "use" a variable to suppress unused variable warnings
// (for when we *do* capture the return value in non-strict mode).
template <typename T>
inline void use(const T& v) {
    // prevent "unused variable" optimizations
    asm volatile("" ::"g"(&v) : "memory");
}

// Move-aware overload
template <typename T>
inline void use(T&& v) {
    asm volatile("" ::"g"(&v) : "memory");
}

// ----------------------------------------------------------------------
// Test functions declared with AG_NODISCARD
// ----------------------------------------------------------------------

AG_NODISCARD constexpr int ret_int_constexpr() {
    return 42;
}

AG_NODISCARD inline double ret_double_inline() {
    return 3.14159;
}

AG_NODISCARD std::unique_ptr<int> make_unique_int() {
    return std::make_unique<int>(100);
}

struct Result {
    int i;
    double d;
};
AG_NODISCARD Result make_result(int i, double d) {
    return {i, d};
}

template <typename T>
AG_NODISCARD T identity(T val) {
    return val;
}

AG_NODISCARD int overloaded() { return 1; }
AG_NODISCARD int overloaded(int) { return 2; }

// Control: function without nodiscard
int plain_int() { return 0; }

// ----------------------------------------------------------------------
// Main Check Routine
// ----------------------------------------------------------------------

static void do_negative_ignores_maybe() {
#ifdef AG_NODISCARD_STRICT
    // The following lines intentionally ignore nodiscard results.
    // On compilers that support [[nodiscard]], these should warn;
    // with -Werror or /we4834, they become errors.
    ret_int_constexpr();
    ret_double_inline();
    make_unique_int();
    make_result(1, 2.0);
    identity<int>(8);
    identity<std::string>("x");
    overloaded();
    overloaded(0);

    // Control: ignoring a non-nodiscard function should be allowed.
    plain_int();
#else
    // In non-strict builds, we still compile the code path, but we "use" results
    // so there are no warnings.
    {
        auto v1 = ret_int_constexpr(); use(v1);
        auto v2 = ret_double_inline(); use(v2);
        auto p  = make_unique_int();   use(p);
        auto r  = make_result(2, 3.0); use(r);
        auto t1 = identity<int>(9);    use(t1);
        auto t2 = identity<std::string>("y"); use(t2);
        auto o1 = overloaded();        use(o1);
        auto o2 = overloaded(0);       use(o2);
        plain_int(); // OK to ignore
    }
#endif
}

// ----------------------------------------------------------------------
// Check AG_NODISCARD_FORCE_OFF
// ----------------------------------------------------------------------
#ifdef AG_NODISCARD_FORCE_OFF
  #undef AG_NODISCARD
  #define AG_NODISCARD /* nothing */
#endif

// Re-declare under the possibly overridden macro to confirm it's harmless.
AG_NODISCARD int sanity_noop() { return 123; }

int main() {
    do_negative_ignores_maybe();
    use(sanity_noop());
    return 0;
}
