# NODISCARD Test Results

## Summary
Successfully tested the `AG_NODISCARD` attribute (which expands to `[[nodiscard]]`) by creating a test file that intentionally ignores `Value` return values.

## Test File Created
- **File**: `Tests/test_nodiscard_bench.cpp`
- **Purpose**: Intentionally ignore `Value` return values to verify that `[[nodiscard]]` warnings are generated

## Key Findings

### ✅ NODISCARD is Working Correctly

The compiler **successfully generated warnings** for all intentionally ignored return values:

1. **Arithmetic Operations** - All triggered warnings:
   - `a + b` (operator+)
   - `a - b` (operator-)
   - `a * b` (operator*)
   - `a / b` (operator/)

2. **Function Calls** - All triggered warnings:
   - `matmul(x, W1)`
   - `sigmoid(a)`
   - `sum(a)`
   - `h1 * h1`, `h1 - h1`, `h1 / h1`

3. **Complex Expressions** - All triggered warnings:
   - `matmul(x, W1) + b1`
   - `sigmoid(matmul(x, W1))`

### Warning Format
The compiler warnings follow this pattern:
```
warning: ignoring returned value of type 'ag::Value', declared with attribute 'nodiscard' [-Wunused-result]
   XX |     a + b;           // WARNING: ignored return value
      |          ^
```

Each warning includes:
- The line number where the value was ignored
- The specific operation that returned the ignored value
- A reference to where `ag::Value` is declared with `AG_NODISCARD`

## Example Warnings from Compilation

Here are some actual warnings from the compilation output:

```
/home/blubridge-041/Downloads/cgadimpl-experiment-11.11.25/cgadimpl/Tests/test_nodiscard_bench.cpp:50:25: warning: ignoring returned value of type 'ag::Value', declared with attribute 'nodiscard' [-Wunused-result]
   50 |         matmul(x, W1);           // Ignored return value
      |                         ^

/home/blubridge-041/Downloads/cgadimpl-experiment-11.11.25/cgadimpl/Tests/test_nodiscard_bench.cpp:98:10: warning: ignoring returned value of type 'ag::Value', declared with attribute 'nodiscard' [-Wunused-result]
   98 |     a + b;           // WARNING: ignored return value
      |          ^

/home/blubridge-041/Downloads/cgadimpl-experiment-11.11.25/cgadimpl/Tests/test_nodiscard_bench.cpp:102:11: warning: ignoring returned value of type 'ag::Value', declared with attribute 'nodiscard' [-Wunused-result]
  102 |     sum(a);          // WARNING: ignored return value
      |           ^

/home/blubridge-041/Downloads/cgadimpl-experiment-11.11.25/cgadimpl/Tests/test_nodiscard_bench.cpp:103:15: warning: ignoring returned value of type 'ag::Value', declared with attribute 'nodiscard' [-Wunused-result]
  103 |     sigmoid(a);      // WARNING: ignored return value
      |               ^

/home/blubridge-041/Downloads/cgadimpl-experiment-11.11.25/cgadimpl/Tests/test_nodiscard_bench.cpp:104:17: warning: ignoring returned value of type 'ag::Value', declared with attribute 'nodiscard' [-Wunused-result]
  104 |     matmul(a, b);    // WARNING: ignored return value
      |                 ^
```

## How AG_NODISCARD Works

From `include/ad/core/nodiscard.hpp`:
```cpp
#ifndef AG_NODISCARD
#  if defined(__has_cpp_attribute)
#    if __has_cpp_attribute(nodiscard)
#      define AG_NODISCARD [[nodiscard]]
#    else
#      define AG_NODISCARD 
#    endif
#  else
#    define AG_NODISCARD 
#  endif
#endif
```

Applied to `Value` in `include/ad/core/graph.hpp`:
```cpp
struct AG_NODISCARD Value { 
    std::shared_ptr<Node> node;
    // ... rest of the struct
};
```

## Conclusion

The `AG_NODISCARD` attribute is **fully functional** and provides excellent protection against accidentally ignoring computational graph nodes. This is critical for the AG framework because:

1. **Graph Correctness**: Every `Value` operation creates a node in the computational graph. Ignoring a return value means that node is lost and won't be part of the gradient computation.

2. **Memory Safety**: Ignored nodes might not be properly tracked, leading to memory leaks or dangling references.

3. **Developer Experience**: The warnings immediately alert developers when they accidentally write code like:
   ```cpp
   matmul(x, W);  // BUG: Result is lost!
   ```
   Instead of:
   ```cpp
   auto result = matmul(x, W);  // Correct: Result is captured
   ```

## Test Execution

The test executable runs successfully and produces no runtime errors:
```
========================================
NODISCARD Test: Checking Compiler Warnings
========================================

This test intentionally ignores Value return values.
If AG_NODISCARD is working, you should see compiler warnings!
Look for warnings like: 'ignoring return value of type 'ag::Value''

Calling forward_broken() which ignores return values...

Additional nodiscard tests:
Correct usage: Value c = a + b (no warning expected)

========================================
Test complete!
Check compilation output for [[nodiscard]] warnings.
========================================
```

The warnings appear during **compilation**, not runtime, which is exactly the intended behavior of `[[nodiscard]]`.
