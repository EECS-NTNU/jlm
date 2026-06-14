---
name: testing
description: JLM compiler unit testing framework. Use when writing, running, or debugging C++ unit tests and C integration tests for the JLM compiler.
---

# JLM Compiler - Unit Testing SKILLS File

## Overview

The JLM compiler uses a dual‑approach testing strategy:

- **C++ Unit Tests**: Google Test framework for testing core RVSDG structures, operations, and utilities
- **C Integration Tests**: End‑to‑end tests that compile C code and verify execution

**Test Directory Structure:**
```
jlm/
├── rvsdg/         # Core RVSDG tests (TestNodes, TestOperations, TestType)
├── util/          # Utility library tests
├── llvm/          # LLVM frontend/backend tests
├── hls/           # HLS‑specific tests
└── mlir/          # MLIR conversion tests

tests/
└── c-tests/       # C integration test suite
```

## Google Test Framework (gtest)

### Basic Test Structure
```cpp
#include <gtest/gtest.h>

TEST(TestSuiteName, test_functionality)
{
  // Arrange
  // Act
  // Assert
}
```

### Common Assertions
- Equality: `EXPECT_EQ`, `ASSERT_EQ`
- Boolean: `EXPECT_TRUE`, `EXPECT_FALSE`
- Pointers: `EXPECT_NONNULL`, `EXPECT_NULL`
- Floating point: `EXPECT_FLOAT_EQ`, `EXPECT_DOUBLE_EQ`

## Build System Integration

### Library Definition (Makefile.macros)
```makefile
libfoo_SOURCES = a.cpp b.cpp
libfoo_HEADERS = a.hpp b.hpp
libfoo_TESTS   = libfoo_Test1.cpp libfoo_Test2.cpp
$(eval $(call common_library,libfoo))
```

### Test Definition
```makefile
run-libfoo-tests_SOURCES = test1.cpp test2.cpp
run-libfoo-tests_LIBS    = libfoo libutil
$(eval $(call common_test,run-libfoo-tests))
```

## Running Tests

- **All tests**: `make check`
- **Valgrind memory checks**: `make valgrind-check`
- **Coverage report**: `./configure --enable-coverage && make coverage`

### Individual Test Execution
```bash
# Run a specific binary
./build/run-librvsdg-tests

# Filter tests
./build/run-librvsdg-tests --gtest_filter=GraphTests.*
```

## Testing Utilities

### Core Test Types
- `TestNodes.hpp/cpp`
- `TestOperations.hpp/cpp`
- `TestType.hpp/cpp`

### Example: Structural Node Test
```cpp
auto node = TestStructuralNode::create(region, 2);
auto input = node->addInputWithArguments(origin);
```

## HLS Backend Unit Testing

### Protected Method Testing
```cpp
class TestableBaseHLS : public jlm::hls::BaseHLS {
public:
  using jlm::hls::BaseHLS::get_port_name;
  // …
  std::string GetText(llvm::LlvmRvsdgModule&) override { return ""; }
  std::string extension() override { return ".txt"; }
};

TEST(HlsTests, TestProtectedMethod) {
  EXPECT_EQ(TestableBaseHLS().get_port_name(input), "a0");
}
```

### BundleType Filtering
```cpp
auto memReq = std::make_shared<jlm::hls::BundleType>(elements);
EXPECT_EQ(TestableBaseHLS().get_mem_reqs(*lambda).size(), 1);
```

## Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| `JlmSize()` throws | Unsupported TestType | Use `BitType::Create(n)` |
| Wrong type in test setup | Using plain struct instead of BundleType | Use `BundleType` for memory types |
| Empty subregion results | Forgot to call `finalize()` | Call `lambda->finalize(...)` before testing |
| Pure virtual methods missing | No overrides for `GetText()` / `extension()` | Provide minimal implementations |

## Test Coverage

- Enable with `./configure --enable-coverage`
- Run `make coverage` → HTML report in `build/coverage/`

## Debugging Tests

- **GDB**: `gdb ./build/run-librvsdg-tests --args --gtest_filter=TestName.*`
- **Verbose output**: `--gtest_brief=0`

## Quick Reference
| Command | Description |
|---------|-------------|
| `make check`           | Run all unit tests |
| `make valgrind-check` | Run tests under Valgrind (memory checking) |
| `make coverage`       | Generate test‑coverage report |
| `make check-headers`  | Verify header consistency |

## FAQ
**Q:** Why does a test pass locally but fail on CI?  
**A:** CI may use a different compiler version or missing dependencies. Reproduce the CI environment locally (`./configure.sh` with same flags) and ensure all required libraries are installed.

**Q:** Missing `gtest` library?  
**A:** Install via package manager (`apt-get install libgtest-dev`) or build from source and point CMake to it.

## SEE ALSO
- **MAIN SKILL**: [Project overview](skill:main)
- **LLVM SKILL**: [LLVM mode details](skill:llvm)
- **HLS SKILL**: [High‑Level Synthesis mode](skill:hls)
- **MLIR SKILL**: [MLIR mode details](skill:mlir)
- **CIRCT SKILL**: [CIRCT integration](skill:circt)