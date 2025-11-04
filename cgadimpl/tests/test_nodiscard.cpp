// // tests/test_nodiscard.cpp
// // Purpose: validate AG_NODISCARD macro behavior across compilers & edge cases.

// #include <memory>
// #include <string>
// #include <type_traits>
// #include <iostream>
// #include <vector>

// // ---- Include your macro header ----
// #include "ad/nodiscard.hpp"  

// // --Value definition header--
// #include "ad/graph.hpp"
//  using ag::Value; 

// // -------------------------------
// // Compile-time feature detection
// // -------------------------------
// #if defined(__has_cpp_attribute)
//   #define AG_TEST_HAS_CPP_ATTR 1
// #else
//   #define AG_TEST_HAS_CPP_ATTR 0
// #endif

// #if AG_TEST_HAS_CPP_ATTR
//   #if __has_cpp_attribute(nodiscard)
//     #define AG_TEST_HAS_NODISCARD 1
//   #else
//     #define AG_TEST_HAS_NODISCARD 0
//   #endif
// #else
//   #define AG_TEST_HAS_NODISCARD 0
// #endif

// // -------------------------------
// // Helpers to "use" a value safely
// // -------------------------------
// template <typename T>
// inline void use(const T& v) {
//   // prevent "unused variable" optimizations
//   asm volatile("" ::"g"(&v) : "memory");
// }

// // Move-aware overload
// template <typename T>
// inline void use(T&& v) {
//   asm volatile("" ::"g"(&v) : "memory");
// }

// // -------------------------------
// // Functions under test
// // -------------------------------

// AG_NODISCARD constexpr int ret_int_constexpr() noexcept { return 42; }

// AG_NODISCARD inline double ret_double_inline() { return 3.14159; }

// AG_NODISCARD std::unique_ptr<int> make_unique_int() {
//   return std::unique_ptr<int>(new int(7));
// }

// struct Result {
//   int x;
//   double y;
// };

// AG_NODISCARD Result make_result(int x, double y) noexcept {
//   return Result{x, y};
// }

// // Template returning different types
// template <class T>
// AG_NODISCARD T identity(T v) {
//   return v;
// }

// // Overloads
// AG_NODISCARD int overloaded() { return 11; }
// AG_NODISCARD std::string overloaded(int) { return "eleven"; }

// // Non-nodiscard control function
// int plain_int() { return 5; }

// // -------------------------------
// // Negative/strict tests:
// //
// // If you compile with -DAG_NODISCARD_STRICT and enable
// // "warnings as errors for nodiscard", the intentional
// // ignored results below should make the build fail on
// // compilers that support [[nodiscard]].
// //
// // * GCC/Clang: add -Werror -Wno-unused-variable
// // * MSVC: add /we4834  (C4834: discarding nodiscard return value)
// // -------------------------------
// static void do_negative_ignores_maybe() {
// #ifdef AG_NODISCARD_STRICT
//   // The following lines intentionally ignore nodiscard results.
//   // On compilers that support [[nodiscard]], these should warn;
//   // with -Werror or /we4834, they become errors.
//   ret_int_constexpr();
//   ret_double_inline();
//   make_unique_int();
//   make_result(1, 2.0);
//   identity<int>(8);
//   identity<std::string>("x");
//   overloaded();
//   overloaded(0);

//   // Control: ignoring a non-nodiscard function should be allowed.
//   plain_int();
// #else
//   // In non-strict builds, we still compile the code path, but we "use" results
//   // so there are no warnings.
//   {
//     auto v1 = ret_int_constexpr(); use(v1);
//     auto v2 = ret_double_inline(); use(v2);
//     auto p  = make_unique_int();   use(p);
//     auto r  = make_result(2, 3.0); use(r);
//     auto t1 = identity<int>(9);    use(t1);
//     auto t2 = identity<std::string>("y"); use(t2);
//     auto o1 = overloaded();        use(o1);
//     auto o2 = overloaded(0);       use(o2);
//     plain_int(); // OK to ignore
//   }
// #endif
// }

// // -------------------------------
// // Simulate "no support" branch:
// //
// // You can compile once with -DAG_NODISCARD_FORCE_OFF to ensure the code
// // still compiles even if AG_NODISCARD expands to nothing.
// // (Your header allows user override by defining AG_NODISCARD earlier.)
// // -------------------------------
// #ifdef AG_NODISCARD_FORCE_OFF
//   #undef AG_NODISCARD
//   #define AG_NODISCARD /* nothing */
// #endif

// // Re-declare under the possibly overridden macro to confirm it’s harmless.
// AG_NODISCARD int sanity_noop() { return 123; } 



// //----------------
// //Value def
// //----------------

// template <typename T>
// inline void use(const T& v) {
// #if defined(__GNUC__) || defined(__clang__)
//   asm volatile("" ::"g"(&v) : "memory");
// #else
//   (void)v;
// #endif
// }

// // Minimal identity template to test template instantiation with Value
// template <class T>
// T identity(T v) { return v; }

// // Overloads returning Value
// Value f();
// Value f(int);

// // Return a Value by value
// Value make_value();

// // Dummy impls if the above aren’t provided by your project.
// // Remove these if you already have real ones in your codebase.
// #ifndef AG_LOCAL_VALUE_IMPL
// Value f()          { return Value{}; }
// Value f(int)       { return Value{}; }
// Value make_value() { return Value{}; }
// #endif

// // -------------------------------
// // Feature/probe printouts
// // -------------------------------
// #if defined(__has_cpp_attribute)
//   #if __has_cpp_attribute(nodiscard) >= 201907L
//     #define AG_TEST_HAS_NODISCARD_TYPE 1
//   #else
//     #define AG_TEST_HAS_NODISCARD_TYPE 0
//   #endif
// #else
//   #define AG_TEST_HAS_NODISCARD_TYPE 0
// #endif

// // -------------------------------
// // STRICT vs NORMAL driver
// // -------------------------------
// static void exercise_negative_path_or_consume()
// {
// #ifdef AG_NODISCARD_STRICT
//   // STRICT: Intentionally DISCARD all Value-producing expressions.
//   // On compilers supporting nodiscard-on-type, these should warn/error
//   // (treat warnings as errors in CI: GCC/Clang -Werror, MSVC /we4834).

//   Value{};                  // discard temporary (default-constructed)
//   make_value();             // discard function return (by value)
//   f();                      // discard overload #1
//   f(0);                     // discard overload #2
//   identity<Value>(Value{}); // discard templated return
//   (void)Value{};            // explicit cast-to-void (still a discard)
//   true ? Value{} : Value{}; // conditional expression producing a temp
//   [](){ return Value{}; }(); // lambda producing a temp that's ignored

//   // If your Value has an explicit ctor like Value(std::shared_ptr<Node>),
//   // and value.hpp brings in Node, you can also trigger:
//   // Value(std::shared_ptr<Node>{});  // discard ctor-built temp

// #else
//   // NORMAL: Consume values so no diagnostics fire.
//   {
//     auto v0 = Value{};                  use(v0);
//     auto v1 = make_value();             use(v1);
//     auto v2 = f();                      use(v2);
//     auto v3 = f(0);                     use(v3);
//     auto v4 = identity<Value>(Value{}); use(v4);
//     auto v5 = [](){ return Value{}; }();use(v5);

//     using V = Value; // alias should not change behavior
//     auto v6 = V{};                        use(v6);

//     // Conditional expression kept (bind then use)
//     auto v7 = true ? Value{} : Value{};   use(v7);

//     // Cast-to-void avoided in NORMAL; we keep & use instead
//   }
// #endif
// }



// // -------------------------------
// // Minimal runtime self-checks:
// //
// // We can't assert "warnings" at runtime, but we can:
// //  * verify functions behave correctly,
// //  * print what capability branch we compiled under.
// // -------------------------------
// int main() {
//   std::cout << "AG_NODISCARD test harness\n";
//   std::cout << "Compiler has __has_cpp_attribute: " << (AG_TEST_HAS_CPP_ATTR ? "yes" : "no") << "\n";
//   std::cout << "Compiler supports [[nodiscard]]   : " << (AG_TEST_HAS_NODISCARD ? "yes" : "no") << "\n";
// #ifdef AG_NODISCARD_STRICT
//   std::cout << "Mode: STRICT (ignoring nodiscard should be an error if supported)\n";
// #else
//   std::cout << "Mode: NORMAL (no intentional ignore errors)\n";
// #endif
// #ifdef AG_NODISCARD_FORCE_OFF
//   std::cout << "AG_NODISCARD forced OFF via override\n";
// #endif

//   // Positive behavior checks (should compile & run everywhere)
//   static_assert(ret_int_constexpr() == 42, "constexpr nodiscard failed");
//   {
//     auto d = ret_double_inline();
//     if (d <= 3.14 || d >= 3.15) {
//       std::cerr << "ret_double_inline value unexpected\n";
//       return 1;
//     }
//   }
//   {
//     auto p = make_unique_int();
//     if (!p || *p != 7) {
//       std::cerr << "make_unique_int value unexpected\n";
//       return 1;
//     }
//   }
//   {
//     auto r = make_result(10, 2.5);
//     if (r.x != 10 || r.y != 2.5) {
//       std::cerr << "make_result value unexpected\n";
//       return 1;
//     }
//   }
//   {
//     auto i = identity<int>(123);
//     auto s = identity<std::string>(std::string("ok"));
//     if (i != 123 || s != "ok") {
//       std::cerr << "identity<T> mismatch\n";
//       return 1;
//     }
//   }
//   {
//     auto a = overloaded();
//     auto b = overloaded(0);
//     if (a != 11 || b != "eleven") {
//       std::cerr << "overloaded mismatch\n";
//       return 1;
//     }
//   }
//   {
//     auto z = sanity_noop();
//     if (z != 123) {
//       std::cerr << "sanity_noop mismatch\n";
//       return 1;
//     }
//   }

//   // Exercise negative path (may warn/error in STRICT mode)
//   do_negative_ignores_maybe();

//   //--------------
//   // VALUE DEF TESTS
//   //--------------
//   std::cout << "Value [[nodiscard]] type test\n";
//   std::cout << "Compiler supports nodiscard-on-type: "
//             << (AG_TEST_HAS_NODISCARD_TYPE ? "yes" : "no") << "\n";
// #ifdef AG_NODISCARD_STRICT
//   std::cout << "Mode: STRICT (discard should diagnose when supported)\n";
// #else
//   std::cout << "Mode: NORMAL (all values consumed; no diagnostics expected)\n";
// #endif

//   exercise_negative_path_or_consume();

//   std::cout << "Type-level nodiscard exercise completed.\n";

  
//   //std::cout << "All runtime checks passed.\n";


//   std::cout << "All done.\n";
//   return 0;
// }


// //tests/test_nodiscard.cpp
// //Purpose:
// //  1) Validate AG_NODISCARD (function-level) behavior across compilers & edge cases.
// //  2) Validate [[nodiscard]] applied to the Value TYPE itself (discarding temporaries/returns).

// #include <memory>
// #include <string>
// #include <type_traits>
// #include <iostream>
// #include <vector>

// // ===============================
// //  Include your macro header
// // ===============================
// #include "ad/nodiscard.hpp"

// // ===============================
// //  (Optional) Include Value type
// //  - By default: #include "value.hpp"
// //  - Or pass: -DAG_VALUE_HEADER=\"ad/value.hpp\"
// // ===============================
// #if defined(AG_VALUE_HEADER)
// #  include AG_VALUE_HEADER
// #else
// #  include "ad/graph.hpp"

// #endif

// // =======================================================
// // ==============  ORIGINAL MACRO TESTS  =================
// // (Unmodified from what you shared; only appended sections below)
// // =======================================================

// // -------------------------------
// // Compile-time feature detection
// // -------------------------------
// #if defined(__has_cpp_attribute)
//   #define AG_TEST_HAS_CPP_ATTR 1
// #else
//   #define AG_TEST_HAS_CPP_ATTR 0
// #endif

// #if AG_TEST_HAS_CPP_ATTR
//   #if __has_cpp_attribute(nodiscard)
//     #define AG_TEST_HAS_NODISCARD 1
//   #else
//     #define AG_TEST_HAS_NODISCARD 0
//   #endif
// #else
//   #define AG_TEST_HAS_NODISCARD 0
// #endif

// // -------------------------------
// // Helpers to "use" a value safely
// // -------------------------------
// template <typename T>
// inline void use(const T& v) {
// #if defined(__GNUC__) || defined(__clang__)
//   // prevent "unused variable" warnings and optimizer elision
//   asm volatile("" ::"g"(&v) : "memory");
// #else
//   (void)v;
// #endif
// }

// // Move-aware overload
// template <typename T>
// inline void use(T&& v) {
// #if defined(__GNUC__) || defined(__clang__)
//   asm volatile("" ::"g"(&v) : "memory");
// #else
//   (void)v;
// #endif
// }

// // -------------------------------
// // Functions under test
// // -------------------------------

// AG_NODISCARD constexpr int ret_int_constexpr() noexcept { return 42; }

// AG_NODISCARD inline double ret_double_inline() { return 3.14159; }

// AG_NODISCARD std::unique_ptr<int> make_unique_int() {
//   return std::unique_ptr<int>(new int(7));
// }

// struct Result {
//   int x;
//   double y;
// };

// AG_NODISCARD Result make_result(int x, double y) noexcept {
//   return Result{x, y};
// }

// // Template returning different types
// template <class T>
// AG_NODISCARD T identity(T v) {
//   return v;
// }

// // Overloads
// AG_NODISCARD int overloaded() { return 11; }
// AG_NODISCARD std::string overloaded(int) { return "eleven"; }

// // Non-nodiscard control function
// int plain_int() { return 5; }

// // -------------------------------
// // Negative/strict tests
// // -------------------------------
// static void do_negative_ignores_maybe() {
// #ifdef AG_NODISCARD_STRICT
//   // Intentionally ignore nodiscard results:
//   ret_int_constexpr();
//   ret_double_inline();
//   make_unique_int();
//   make_result(1, 2.0);
//   identity<int>(8);
//   identity<std::string>("x");
//   overloaded();
//   overloaded(0);

//   // Control: ignoring a non-nodiscard function should be allowed.
//   plain_int();
// #else
//   // Non-strict: consume values so there are no warnings.
//   {
//     auto v1 = ret_int_constexpr(); use(v1);
//     auto v2 = ret_double_inline(); use(v2);
//     auto p  = make_unique_int();   use(p);
//     auto r  = make_result(2, 3.0); use(r);
//     auto t1 = identity<int>(9);    use(t1);
//     auto t2 = identity<std::string>("y"); use(t2);
//     auto o1 = overloaded();        use(o1);
//     auto o2 = overloaded(0);       use(o2);
//     plain_int(); // OK to ignore
//   }
// #endif
// }

// // -------------------------------
// // Simulate "no support" branch
// // -------------------------------
// #ifdef AG_NODISCARD_FORCE_OFF
//   #undef AG_NODISCARD
//   #define AG_NODISCARD /* nothing */
// #endif

// // Re-declare under the possibly overridden macro to confirm it’s harmless.
// AG_NODISCARD int sanity_noop() { return 123; }

// // =======================================================
// // =======  APPENDED: TYPE-LEVEL [[nodiscard]] TESTS  =====
// //   These validate nodiscard on the Value type itself.
// //   Expect a warning/error (STRICT + supported compiler)
// //   when discarding temporaries or returned Value.
// // =======================================================

// // Probe whether compiler claims to support nodiscard-on-type (C++20+)
// #if defined(__has_cpp_attribute)
//   #if __has_cpp_attribute(nodiscard) >= 201907L
//     #define AG_TEST_HAS_NODISCARD_TYPE 1
//   #else
//     #define AG_TEST_HAS_NODISCARD_TYPE 0
//   #endif
// #else
//   #define AG_TEST_HAS_NODISCARD_TYPE 0
// #endif

// // If your codebase already provides these, define -DAG_HAS_VALUE_FACTORIES
// #ifndef AG_HAS_VALUE_FACTORIES
// // Minimal factories returning Value by value.
// // These assume Value is default-constructible.
// static Value make_value() { return Value{}; }
// static Value f()         { return Value{}; }
// static Value f(int)      { return Value{}; }
// #endif

// static void exercise_value_type_discard_or_consume()
// {
// #ifdef AG_NODISCARD_STRICT
//   // STRICT: Discard a bunch of Value-producing expressions.
//   // On compilers that support nodiscard on the type, these should warn/error
//   // (use -Werror or /we4834 in CI to turn warnings into errors).

//   Value{};                    // discard temporary
//   make_value();               // discard function return
//   f();                        // discard overload #1
//   f(0);                       // discard overload #2
//   identity<Value>(Value{});   // discard templated return
//   (void)Value{};              // explicit cast-to-void still discards
//   true ? Value{} : Value{};   // conditional expression producing a temp
//   [](){ return Value{}; }();  // lambda returning a temp, ignored

// #else
//   // NORMAL: Consume everything; no diagnostics expected.
//   auto v0 = Value{};                    use(v0);
//   auto v1 = make_value();               use(v1);
//   auto v2 = f();                        use(v2);
//   auto v3 = f(0);                       use(v3);
//   auto v4 = identity <Value>(Value());   use(v4);
//   auto v5 = [](){ return Value(); }();  use(v5);

//   using V = Value{}; // alias shouldn’t change behavior
//   auto v6 = V{};                         use(v6);

//   auto v7 = true ? Value() : Value();    use(v7);
// #endif
// }

// // =======================================================
// // =====================  MAIN  ==========================
// // =======================================================
// int main() {
//   std::cout << "AG_NODISCARD test harness\n";
//   std::cout << "Compiler has __has_cpp_attribute: " << (AG_TEST_HAS_CPP_ATTR ? "yes" : "no") << "\n";
//   std::cout << "Compiler supports [[nodiscard]]   : " << (AG_TEST_HAS_NODISCARD ? "yes" : "no") << "\n";
//   std::cout << "Compiler supports nodiscard-on-type: " << (AG_TEST_HAS_NODISCARD_TYPE ? "yes" : "no") << "\n";
// #ifdef AG_NODISCARD_STRICT
//   std::cout << "Mode: STRICT (ignoring nodiscard should be an error if supported)\n";
// #else
//   std::cout << "Mode: NORMAL (no intentional ignore errors)\n";
// #endif
// #ifdef AG_NODISCARD_FORCE_OFF
//   std::cout << "AG_NODISCARD forced OFF via override\n";
// #endif

//   // ------- Original positive behavior checks -------
//   static_assert(ret_int_constexpr() == 42, "constexpr nodiscard failed");
//   {
//     auto d = ret_double_inline();
//     if (d <= 3.14 || d >= 3.15) {
//       std::cerr << "ret_double_inline value unexpected\n";
//       return 1;
//     }
//   }
//   {
//     auto p = make_unique_int();
//     if (!p || *p != 7) {
//       std::cerr << "make_unique_int value unexpected\n";
//       return 1;
//     }
//   }
//   {
//     auto r = make_result(10, 2.5);
//     if (r.x != 10 || r.y != 2.5) {
//       std::cerr << "make_result value unexpected\n";
//       return 1;
//     }
//   }
//   {
//     auto i = identity<int>(123);
//     auto s = identity<std::string>(std::string("ok"));
//     if (i != 123 || s != "ok") {
//       std::cerr << "identity<T> mismatch\n";
//       return 1;
//     }
//   }
//   {
//     auto a = overloaded();
//     auto b = overloaded(0);
//     if (a != 11 || b != "eleven") {
//       std::cerr << "overloaded mismatch\n";
//       return 1;
//     }
//   }
//   {
//     auto z = sanity_noop();
//     if (z != 123) {
//       std::cerr << "sanity_noop mismatch\n";
//       return 1;
//     }
//   }

//   // ------- Original diagnostic exercise (STRICT/NORMAL) -------
//   do_negative_ignores_maybe();

//   // ------- Appended: Value type-level diagnostic exercise -------
//   exercise_value_type_discard_or_consume();

//   std::cout << "All runtime checks passed.\n";
//   return 0;
// }
// ---- standard ----








// #include <iostream>
// #include <string>
// #include <vector>

// ---- project: adjust include path as needed ----
//#include "ad/graph.hpp"
//using ag::Value;   // must define: struct [[nodiscard]] Value { ... };

// // Optional generic sink to mark a value as "used" (non-STRICT mode)
// template <typename T>
// inline void use(const T& v) {
// #if defined(__GNUC__) || defined(__clang__)
//   asm volatile("" ::"g"(&v) : "memory");
// #else
//   (void)v;
// #endif
// }

// // Minimal identity template to test template instantiation with Value
// template <class T>
// T identity(T v) { return v; }

// // Overloads returning Value
// Value f();
// Value f(int);

// // Return a Value by value
// Value make_value();

// // Dummy impls if the above aren’t provided by your project.
// // Remove these if you already have real ones in your codebase.
// #ifndef AG_LOCAL_VALUE_IMPL
// Value f()          { return Value{}; }
// Value f(int)       { return Value{}; }
// Value make_value() { return Value{}; }
// #endif

// // -------------------------------
// // Feature/probe printouts
// // -------------------------------
// #if defined(__has_cpp_attribute)
//   #if __has_cpp_attribute(nodiscard) >= 201907L
//     #define AG_TEST_HAS_NODISCARD_TYPE 1
//   #else
//     #define AG_TEST_HAS_NODISCARD_TYPE 0
//   #endif
// #else
//   #define AG_TEST_HAS_NODISCARD_TYPE 0
// #endif

// // -------------------------------
// // STRICT vs NORMAL driver
// // -------------------------------
// static void exercise_negative_path_or_consume()
// {
// #ifdef AG_NODISCARD_STRICT
//   // STRICT: Intentionally DISCARD all Value-producing expressions.
//   // On compilers supporting nodiscard-on-type, these should warn/error
//   // (treat warnings as errors in CI: GCC/Clang -Werror, MSVC /we4834).

//   Value{};                  // discard temporary (default-constructed)
//   make_value();             // discard function return (by value)
//   f();                      // discard overload #1
//   f(0);                     // discard overload #2
//   identity<Value>(Value{}); // discard templated return
//   (void)Value{};            // explicit cast-to-void (still a discard)
//   true ? Value{} : Value{}; // conditional expression producing a temp
//   [](){ return Value{}; }(); // lambda producing a temp that's ignored

//   // If your Value has an explicit ctor like Value(std::shared_ptr<Node>),
//   // and value.hpp brings in Node, you can also trigger:
//   // Value(std::shared_ptr<Node>{});  // discard ctor-built temp

// #else
//   // NORMAL: Consume values so no diagnostics fire.
//   {
//     auto v0 = Value{};                  use(v0);
//     auto v1 = make_value();             use(v1);
//     auto v2 = f();                      use(v2);
//     auto v3 = f(0);                     use(v3);
//     auto v4 = identity<Value>(Value{}); use(v4);
//     auto v5 = [](){ return Value{}; }();use(v5);

//     using V = Value; // alias should not change behavior
//     auto v6 = V{};                        use(v6);

//     // Conditional expression kept (bind then use)
//     auto v7 = true ? Value{} : Value{};   use(v7);

//     // Cast-to-void avoided in NORMAL; we keep & use instead
//   }
// #endif
// }

// -------------------------------
// Minimal runtime harness
// (Type-level nodiscard has no "value" to assert; we just run paths.)
// -------------------------------
// int main()
// {
//   std::cout << "Value [[nodiscard]] type test\n";
//   std::cout << "Compiler supports nodiscard-on-type: "
//             << (AG_TEST_HAS_NODISCARD_TYPE ? "yes" : "no") << "\n";
// #ifdef AG_NODISCARD_STRICT
//   std::cout << "Mode: STRICT (discard should diagnose when supported)\n";
// #else
//   std::cout << "Mode: NORMAL (all values consumed; no diagnostics expected)\n";
// #endif

//   exercise_negative_path_or_consume();

//   std::cout << "Type-level nodiscard exercise completed.\n";
//   return 0;
// }


// -------------------------------
// UPDATE 
//--------------------------------

// tests/test_nodiscard.cpp
// Purpose: validate AG_NODISCARD (function-level) and [[nodiscard]] on Value (type-level).

#include <memory>
#include <string>
#include <type_traits>
#include <iostream>
#include <vector>

// ---- Project headers ----
#include "ad/nodiscard.hpp"
#include "ad/graph.hpp"
using ag::Value;

// -------------------------------
// One portable sink to "use" values
// -------------------------------
template <typename T>
inline void use_any(const T& v) {
#if defined(__GNUC__) || defined(__clang__)
  asm volatile("" ::"g"(&v) : "memory");
#else
  (void)v;
#endif
}

// -------------------------------
// Compile-time feature detection (function-level)
// -------------------------------
#if defined(__has_cpp_attribute)
  #define AG_TEST_HAS_CPP_ATTR 1
#else
  #define AG_TEST_HAS_CPP_ATTR 0
#endif

#if AG_TEST_HAS_CPP_ATTR
  #if __has_cpp_attribute(nodiscard)
    #define AG_TEST_HAS_NODISCARD 1
  #else
    #define AG_TEST_HAS_NODISCARD 0
  #endif
#else
  #define AG_TEST_HAS_NODISCARD 0
#endif

// ===============================
// Suite A: Function-level tests
// ===============================
namespace fn {

// Functions under test
AG_NODISCARD constexpr int ret_int_constexpr() noexcept { return 42; }
AG_NODISCARD inline double ret_double_inline() { return 3.14159; }
AG_NODISCARD std::unique_ptr<int> make_unique_int() {
  return std::unique_ptr<int>(new int(7));
}

struct Result { int x; double y; };
AG_NODISCARD Result make_result(int x, double y) noexcept { return Result{x, y}; }

// Template returning different types (function-level attribute)
template <class T>
AG_NODISCARD T identity(T v) { return v; }

// Overloads
AG_NODISCARD int overloaded() { return 11; }
AG_NODISCARD std::string overloaded(int) { return "eleven"; }

// Non-nodiscard control
inline int plain_int() { return 5; }

static void do_negative_ignores_maybe() {
#ifdef AG_NODISCARD_STRICT
  ret_int_constexpr();
  ret_double_inline();
  make_unique_int();
  make_result(1, 2.0);
  identity<int>(8);
  identity<std::string>("x");
  overloaded();
  overloaded(0);
  plain_int(); // allowed to ignore
#else
  auto v1 = ret_int_constexpr(); use_any(v1);
  auto v2 = ret_double_inline(); use_any(v2);
  auto p  = make_unique_int();   use_any(p);
  auto r  = make_result(2, 3.0); use_any(r);
  auto t1 = identity<int>(9);    use_any(t1);
  auto t2 = identity<std::string>("y"); use_any(t2);
  auto o1 = overloaded();        use_any(o1);
  auto o2 = overloaded(0);       use_any(o2);
  plain_int(); // OK to ignore
#endif
}

#ifdef AG_NODISCARD_FORCE_OFF
  #undef AG_NODISCARD
  #define AG_NODISCARD /* nothing */
#endif

AG_NODISCARD int sanity_noop() { return 123; }

} // namespace fn

// ===============================
// Suite B: Type-level [[nodiscard]] on Value
// ===============================

// Feature/probe for type-level C++20 nodiscard
#if defined(__has_cpp_attribute)
  #if __has_cpp_attribute(nodiscard) >= 201907L
    #define AG_TEST_HAS_NODISCARD_TYPE 1
  #else
    #define AG_TEST_HAS_NODISCARD_TYPE 0
  #endif
#else
  #define AG_TEST_HAS_NODISCARD_TYPE 0
#endif

namespace vt {

// Minimal identity template to test template instantiation with Value (separate name)
template <class T>
T id(T v) { return v; }

// Overloads returning Value
Value f();
Value f(int);

// Return a Value by value
Value make_value();

// Dummy impls if project doesn’t provide them
#ifndef AG_LOCAL_VALUE_IMPL
inline Value f()          { return Value{}; }
inline Value f(int)       { return Value{}; }
inline Value make_value() { return Value{}; }
#endif

static void exercise_negative_path_or_consume() {
#ifdef AG_NODISCARD_STRICT
  Value{};                   // discard temporary
  make_value();              // discard function return
  f();                       // discard overload #1
  f(0);                      // discard overload #2
  id<Value>(Value{});        // discard templated return
  (void)Value{};             // explicit cast-to-void
  true ? Value{} : Value{};  // conditional expression
  [](){ return Value{}; }(); // lambda producing temp
#else
  auto v0 = Value{};                   use_any(v0);
  auto v1 = make_value();              use_any(v1);
  auto v2 = f();                       use_any(v2);
  auto v3 = f(0);                      use_any(v3);
  auto v4 = id<Value>(Value{});        use_any(v4);
  auto v5 = [](){ return Value{}; }(); use_any(v5);
  using V = Value; auto v6 = V{};      use_any(v6);
  auto v7 = true ? Value{} : Value{};  use_any(v7);
#endif
}

} // namespace vt

// ===============================
// Main harness
// ===============================
int main() {
  //std::cout << "Type-level nodiscard exercise completed.\n";
  std::cout << "AG_NODISCARD test harness\n";
  std::cout << "Compiler has __has_cpp_attribute: "
            << (AG_TEST_HAS_CPP_ATTR ? "yes" : "no") << "\n";
  std::cout << "Compiler supports [[nodiscard]] (func-level): "
            << (AG_TEST_HAS_NODISCARD ? "yes" : "no") << "\n";
#ifdef AG_NODISCARD_STRICT
  std::cout << "Mode: STRICT (ignoring nodiscard should diagnose)\n";
#else
  std::cout << "Mode: NORMAL (values consumed; no diagnostics expected)\n";
#endif
#ifdef AG_NODISCARD_FORCE_OFF
  std::cout << "AG_NODISCARD forced OFF via override\n";
#endif

  // Function-level positive checks
  static_assert(fn::ret_int_constexpr() == 42, "constexpr nodiscard failed");
  {
    auto d = fn::ret_double_inline();
    if (d <= 3.14 || d >= 3.15) { std::cerr << "ret_double_inline unexpected\n"; return 1; }
  }
  {
    auto p = fn::make_unique_int();
    if (!p || *p != 7) { std::cerr << "make_unique_int unexpected\n"; return 1; }
  }
  {
    auto r = fn::make_result(10, 2.5);
    if (r.x != 10 || r.y != 2.5) { std::cerr << "make_result unexpected\n"; return 1; }
  }
  {
    auto i = fn::identity<int>(123);
    auto s = fn::identity<std::string>(std::string("ok"));
    if (i != 123 || s != "ok") { std::cerr << "identity<T> mismatch\n"; return 1; }
  }
  {
    auto a = fn::overloaded();
    auto b = fn::overloaded(0);
    if (a != 11 || b != "eleven") { std::cerr << "overloaded mismatch\n"; return 1; }
  }
  {
    auto z = fn::sanity_noop();
    if (z != 123) { std::cerr << "sanity_noop mismatch\n"; return 1; }
  }

  // Exercise function-level negative path
  fn::do_negative_ignores_maybe();

  // Type-level section header
  std::cout << "Value [[nodiscard]] type test\n";
  std::cout << "Compiler supports nodiscard-on-type: "
            << (AG_TEST_HAS_NODISCARD_TYPE ? "yes" : "no") << "\n";

  // Exercise type-level cases
  vt::exercise_negative_path_or_consume();

  std::cout << "Type-level nodiscard exercise completed.\n";
  std::cout << "All runtime checks passed.\n"; 
  return 0;
} 