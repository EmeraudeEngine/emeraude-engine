# Testing System

Context for developing Emeraude Engine unit tests.

## Module Overview

Engine unit tests using **Google Test**. Current focus on **Libs** (critical foundation), future expansion to high-level systems.

## Testing-Specific Rules

### Framework: Google Test
- **Google Test (gtest)**: C++ unit testing framework
- **CTest integration**: Launched via `ctest` for CI/CD
- **Standard assertions**: EXPECT_*, ASSERT_*, etc.

### MANDATORY Test Conventions

**File organization**:
- **One file per class/concept**: One test file per tested unit
- Naming: `ClassNameTest.cpp` or `ConceptTest.cpp`

**Test organization**:
- **One test per function**: Each function has its own TEST()
- **Variants in same test**: Alternative calls/edge cases grouped
- Test naming: `TEST(ClassName, FunctionName)` or `TEST(ClassName, FunctionName_EdgeCase)`

**Example**:
```cpp
// VectorTest.cpp - tests Libs/Math/Vector

TEST(Vector, Constructor) {
    Vector3 v1(1.0f, 2.0f, 3.0f);
    EXPECT_EQ(v1.x, 1.0f);
    EXPECT_EQ(v1.y, 2.0f);
    EXPECT_EQ(v1.z, 3.0f);

    // Variant: default constructor
    Vector3 v2;
    EXPECT_EQ(v2.x, 0.0f);
    EXPECT_EQ(v2.y, 0.0f);
    EXPECT_EQ(v2.z, 0.0f);
}

TEST(Vector, Length) {
    Vector3 v(3.0f, 4.0f, 0.0f);
    EXPECT_FLOAT_EQ(v.length(), 5.0f);

    // Edge case: zero vector
    Vector3 zero(0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(zero.length(), 0.0f);
}

TEST(Vector, Normalize) {
    Vector3 v(3.0f, 4.0f, 0.0f);
    v.normalize();
    EXPECT_FLOAT_EQ(v.length(), 1.0f);
    EXPECT_FLOAT_EQ(v.x, 0.6f);
    EXPECT_FLOAT_EQ(v.y, 0.8f);

    // Edge case: normalize zero vector
    Vector3 zero(0.0f, 0.0f, 0.0f);
    zero.normalize();  // Must not crash
    EXPECT_TRUE(std::isnan(zero.x) || zero.x == 0.0f);
}
```

### Test Priority

**Currently tested**:
- **Libs**: Absolute priority (engine foundation)
  - Math (Vector, Matrix, Quaternion, CartesianFrame)
  - Algorithms
  - IO
  - ThreadPool
  - Observer/Observable
  - Etc.

**Future expansion**:
- Resources (loading, fail-safe, dependencies)
- Physics (collisions, constraints, integration)
- Graphics (geometry, materials, renderables)
- Scenes (nodes, components, transformations)
- Saphir (shader generation, compatibility)

### Stack-Up Strategy
1. **Libs 100% tested**: Solid foundation guaranteed
2. **Foundational systems**: Resources, Physics
3. **Graphics systems**: Graphics, Saphir
4. **High-level systems**: Scenes, Audio, Overlay
5. **Integration tests**: Inter-system tests

## Development Commands

```bash
# Run all tests
ctest

# Run tests in parallel
ctest --parallel $(nproc)

# Run specific tests
ctest -R Vector           # Tests containing "Vector"
ctest -R Libs             # All Libs tests
./test --gtest_filter="Vector.*"  # Google Test filter

# Verbose mode
ctest --verbose
ctest --output-on-failure

# Run test executable directly
./test                    # All tests
./test --gtest_list_tests # List available tests
```

## Important Files

### Testing/ Structure
```
Testing/
├── Libs/                  # Libs tests (current priority)
│   ├── Math/
│   │   ├── VectorTest.cpp
│   │   ├── MatrixTest.cpp
│   │   ├── QuaternionTest.cpp
│   │   └── CartesianFrameTest.cpp
│   ├── IO/
│   │   └── FileSystemTest.cpp
│   ├── ThreadPoolTest.cpp
│   └── ObserverTest.cpp
├── Resources/             # Future
├── Physics/               # Future
└── CMakeLists.txt
```

## Development Patterns

### Creating a New Test
```cpp
// 1. Create file Testing/Category/ClassTest.cpp
#include <gtest/gtest.h>
#include "Libs/Math/Vector.hpp"

// 2. One test per function
TEST(Vector, Add) {
    Vector3 a(1, 2, 3);
    Vector3 b(4, 5, 6);
    Vector3 result = a + b;

    EXPECT_EQ(result.x, 5);
    EXPECT_EQ(result.y, 7);
    EXPECT_EQ(result.z, 9);
}

// 3. Test edge cases
TEST(Vector, Add_WithZero) {
    Vector3 a(1, 2, 3);
    Vector3 zero(0, 0, 0);
    Vector3 result = a + zero;

    EXPECT_EQ(result, a);
}

// 4. Test boundary cases
TEST(Vector, Add_Overflow) {
    Vector3 a(FLT_MAX, 0, 0);
    Vector3 b(1, 0, 0);
    Vector3 result = a + b;
    // Verify overflow behavior
}
```

### Google Test Assertion Types
```cpp
// Equality
EXPECT_EQ(a, b);      // a == b
EXPECT_NE(a, b);      // a != b

// Comparison
EXPECT_LT(a, b);      // a < b
EXPECT_LE(a, b);      // a <= b
EXPECT_GT(a, b);      // a > b
EXPECT_GE(a, b);      // a >= b

// Floats (with epsilon)
EXPECT_FLOAT_EQ(a, b);   // ~equal for float
EXPECT_DOUBLE_EQ(a, b);  // ~equal for double
EXPECT_NEAR(a, b, eps);  // |a - b| <= eps

// Booleans
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// Pointers
EXPECT_EQ(ptr, nullptr);
EXPECT_NE(ptr, nullptr);

// ASSERT_* variants: stop test on failure
ASSERT_EQ(a, b);  // If fails, stops the test
```

### Tests with Fixtures (setup/teardown)
```cpp
class VectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup before each test
        v1 = Vector3(1, 2, 3);
        v2 = Vector3(4, 5, 6);
    }

    void TearDown() override {
        // Cleanup after each test
    }

    Vector3 v1, v2;
};

TEST_F(VectorTest, Add) {
    // Uses v1 and v2 from fixture
    Vector3 result = v1 + v2;
    EXPECT_EQ(result.x, 5);
}
```

## Critical Points

- **Libs priority**: Critical foundation, must be 100% tested
- **One test = one function**: Clarity and isolation
- **Edge cases mandatory**: Test boundary values, zero, negative, overflow
- **EXPECT vs ASSERT**: EXPECT continues after failure, ASSERT stops
- **Float comparison**: Use EXPECT_FLOAT_EQ, not EXPECT_EQ
- **Fast tests**: Avoid long tests (< 1 second ideally)
- **No randomness**: Reproducible tests, no random values
- **CI/CD integration**: Tests run automatically on commits

## Detailed Documentation

Tested systems:
- @src/Libs/AGENTS.md - Current test priority
- Google Test documentation - For assertions and advanced features
- CTest documentation - For CMake integration
