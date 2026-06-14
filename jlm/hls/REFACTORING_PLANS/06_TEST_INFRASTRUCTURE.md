# Phase 6: End-to-End Validation (Ongoing)

## Document Information
- **Phase**: 6 (E2E Validation)
- **Priority**: HIGH - Quality assurance
- **Status**: Updated with Analysis Findings
- **Last Updated**: June 14, 2026

---

## A. E2E Validation Requirements Added - June 14, 2026

This phase addresses missing end-to-end validation strategy identified in refactoring analysis:

#### Missing Components from Original Phase 6
- FIRRTL → Verilog lowering tests (using CIRCT)
- Output equivalence verification (refactored vs original should match)
- Hardware-level validation (Verilator simulation)

#### New E2E Validation Goals
1. Verify FIRRTL output can be lowered to valid Verilog
2. Ensure refactoring doesn't change hardware semantics
3. Detect performance regressions in generated code

---

## B. Original Phase 6 Content (Unit Testing Guidelines)

### Coding Style Requirements

All HLS test code must follow jlm's coding conventions:
- Run `clang-format` before committing (uses `.clang-format` from project root)
- Run `clang-tidy` for static analysis (uses `.clang-tidy` from project root)
- Follow naming conventions: `PascalCase` for classes, `camelCase` for variables
- Use `jlm::hls` namespace for all HLS code

See also: Phase 1 (Setup) for build and test setup instructions.

---

## C. Analysis of `run-hls-test.sh` External Test Suite

### Overview

The script `scripts/run-hls-test.sh --parallel 16` runs an external benchmark suite from the [phate/hls-test-suite](https://github.com/phate/hls-test-suite) repository. It:
1. Clones a separate test suite repository
2. Compiles generated Verilog with Verilator and runs simulations
3. Validates against "golden" cycle counts stored in `.cycles` files

### Key Findings from External Suite Analysis

#### 1. BaseHLS Utility Class (Already Tested)

The `jlm/hls/backend/rhls2firrtl/base-hls.hpp/cpp` provides reusable utilities:
- `get_node_name()`, `create_node_names()` - Consistent naming for tests
- `get_port_name()` - Port naming utilities  
- `get_mem_reqs()`, `get_mem_resps()` - Memory interface extraction
- `get_reg_args()`, `get_reg_results()` - Register port extraction

#### 2. RhlsToFirrtlConverter String Output (Already Tested)

The `RhlsToFirrtlConverter::ToString()` method allows:
- Unit testing FIRRTL generation without external compilation
- Pattern matching on FIRRTL output (e.g., `when`, `else`, `mem`)
- Parameterized tests for different bit widths

#### 3. Transformation Interface Consistency

All transformations use a common interface:
```cpp
StatisticsCollector statisticsCollector;
Transformation::CreateAndRun(rvsdgModule, statisticsCollector);
```

This allows testing transformations in isolation without full pipeline execution.

### Existing Test Coverage (17 test source files registered in Makefile.sub)

| Category | Files | Tests (approx) |
|----------|-------|----------------|
| Base HLS Utils | `BaseHlsTests.cpp` | 24 |
| Gamma/Theta Conversion | `GammaTests.cpp`, `ThetaTests.cpp` | 4+ |
| Memory Conversion | `MemoryConverterTests.cpp` | 7 |
| Memory Queue | `MemoryQueueTests.cpp` | 3 |
| Unused State Removal | `UnusedStateRemovalTests.cpp` | 6 |
| Dead Node Elimination | `DeadNodeEliminationTests.cpp` | 2 |
| Fork/Sink Insertion | `ForkTests.cpp`, `SinkInsertionTests.cpp` | 4+ |
| Stream Conversion | `StreamConversionTests.cpp` | 3 |
| Buffer/Reducant Buffer | `RedundantBufferEliminationTests.cpp` | 9 |
| Distribute Constants | `DistributeConstantsTests.cpp` | 5 |

### Missing Test Categories (Gaps vs External Suite)

1. **FIRRTL to Verilog Lowering Tests** - No tests for `FirrtlToVerilogConverter`
2. **Verilator Harness Generation Tests** - Source exists (`verilator-harness-hls.cpp`) but no tests
3. **AXI Interface Harness Tests** - `VerilatorHarnessAxi` has no test coverage
4. **JSON Output Tests** - `json-hls.cpp` has no tests
5. **Integration/Pipeline Tests** - No end-to-end tests across multiple stages

### Recommendations for Improving Unit/Integration Testing (Excludes Harness Generation)

#### High Priority - Unit Tests That Don't Require Full Pipeline:

1. **FIRRTL String Parsing Tests**
   - Use `RhlsToFirrtlConverter::ToString()` to generate FIRRTL
   - Test specific patterns: `when`, `else`, `mem`, `circuit`
   - Already partially implemented but needs expansion

2. **Transformation Pass Tests (Isolated)**
   - Each transformation (`ForkInsertion`, `BufferInsertion`, etc.) can be tested independently
   - Use the same approach as existing tests: create RVSDG module, run transformation, verify result
   - No need to generate FIRRTL/Verilog

3. **BaseHLS Utility Tests**
   - Test node naming consistency across multiple runs
   - Test port name generation for various type combinations

#### Medium Priority - Integration Tests Without External Suite:

1. **RVSDG → R-HLS Pipeline Test**
   - Test Gamma conversion + Buffer insertion + Fork insertion chain
   - Verify intermediate state after each transformation
   - Use existing tests as templates (e.g., `ForkTests.cpp`)

2. **R-HLS → FIRRTL Integration Tests**
   - Combine transformations with FIRRTL generation
   - Verify FIRRTL structure before running full Verilator simulation

#### Low Priority - External Suite Alternatives:

1. **Verilator Harness Unit Tests** (if external suite unavailable)
   - Test harness generation without Verilator compilation
   - Validate C++ string output matches expected format
   - Use `VerilatorHarnessHLS::GetText()` for unit tests

---

## D. Integration with Existing Refactoring Plans

### RVSDG → R-HLS Conversion Phase (Phase 2)
The test analysis confirms that transformation pass tests in `rvsdg2rhls` are comprehensive:
- `GammaConversion.cpp` - Well tested via `GammaTests.cpp`
- `ThetaConversion.cpp` - Well tested via `ThetaTests.cpp`
- Memory conversion - Covered by `MemoryConverterTests.cpp`

### R-HLS → FIRRTL Conversion Phase (Phase 3)
The test analysis reveals gaps in FIRRTL verification:
- Basic FIRRTL generation is well-tested (`RhlsToFirrtlConverterTests.cpp`)
- Control flow patterns are covered (`RhlsToFirrtlConverterTests_Control.cpp`)
- Memory patterns need expansion (`RhlsToFirrtlConverterTests_Memory.cpp`)

### E2E Validation Phase (Phase 6)
The external suite's main contribution is **end-to-end cycle-accurate simulation**. This can be complemented by:
1. Faster unit tests for individual components
2. Integration tests that chain transformations
3. FIRRTL verification without full Verilator compilation

---

## E. Test Organization Structure

```
jlm/hls/backend/tests/
├── fixtures/                    # Common test fixtures
│   ├── BaseHlsTestFixture.hpp/cpp      # Base HLS testing setup
│   ├── FirrtlVerifyingFixture.hpp/cpp  # FIRRTL output verification
│   └── RvsdgFixture.hpp/cpp            # RVSDG module creation
├── patterns/                    # Reusable test patterns
│   ├── ControlFlowPatterns.cpp         # Gamma/Theta tests
│   ├── MemoryAccessPatterns.cpp        # Load/store tests
│   └── BusProtocolPatterns.cpp         # Ready/valid pattern tests
└── integration/                 # End-to-end tests
    ├── PipelineIntegrationTests.cpp    # Full pipeline tests
    └── RegressionTests.cpp             # Regression protection
```

---

## F. Test Fixture Design

### 3.1 Base HLS Test Fixture

**Note**: All test fixtures use `llvm::LlvmRvsdgModule` as the primary module type for HLS tests.

```cpp
// fixtures/BaseHlsTestFixture.hpp
class BaseHlsTestFixture : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    // Module creation helpers using llvm::LlvmRvsdgModule
    std::unique_ptr<llvm::LlvmRvsdgModule> CreateSimpleAddModule();
    std::unique_ptr<llvm::LlvmRvsdgModule> CreateSimpleMulModule();

    // Helper to get HLS lambda from module
    const rvsdg::LambdaNode* GetHlsLambda(llvm::LlvmRvsdgModule& module);

private:
    std::unique_ptr<mlir::MLIRContext> mlirContext_;
};

// fixtures/BaseHlsTestFixture.cpp
void BaseHlsTestFixture::SetUp() {
    // Initialize MLIR context for FIRRTL generation
    mlirContext_ = std::make_unique<mlir::MLIRContext>();
}

void BaseHlsTestFixture::TearDown() {
    // Clean up resources
}
```

### 3.2 FIRRTL Verification Fixture

```cpp
// fixtures/FirrtlVerifyingFixture.hpp
class FirrtlVerifyingFixture : public ::testing::Test {
protected:
    void SetUp() override;

    // Verify FIRRTL output matches expected pattern
    bool VerifyFirrtlPattern(
        const std::string& output,
        const std::regex& pattern);

    // Check if FIRRTL contains specific operation
    bool ContainsOperation(const std::string& output, const char* opName);

    // Extract module name from FIRRTL
    std::string GetModuleName(const std::string& output);
};
```

### 3.3 RVSDG Fixture

```cpp
// fixtures/RvsdgFixture.hpp
class RvsdgFixture : public ::testing::Test {
protected:
    std::unique_ptr<llvm::LlvmRvsdgModule> module_;

    // Create a simple module with add operation
    rvsdg::Node* AddAddNode(
        rvsdg::Region& region,
        size_t bits = 32);

    // Create a gamma node for control flow tests
    rvsdg::GammaNode* AddGammaNode(rvsdg::Region& region, size_t nalternatives = 2);
};
```

---

## G. Test Pattern Examples

### 4.1 Control Flow Patterns

```cpp
// patterns/ControlFlowPatterns.cpp
namespace jlm::hls::test {

class GammaPattern {
public:
    // Create a gamma node with constant inputs
    static std::unique_ptr<llvm::LlvmRvsdgModule> CreateWithConstants(
        size_t nalternatives,
        int64_t value);

    // Verify the gamma was converted to mux
    static bool IsMux(const rvsdg::Node* node);
};

class ThetaPattern {
public:
    // Create a theta loop with specific iteration count
    static std::unique_ptr<llvm::LlvmRvsdgModule> CreateLoop(
        size_t iterations,
        size_t step = 1);

    // Verify the loop structure is correct
    static bool IsValidLoop(const rvsdg::Node* node);
};

}
```

### 4.2 Memory Access Patterns

```cpp
// patterns/MemoryAccessPatterns.cpp
namespace jlm::hls::test {

class LoadPattern {
public:
    // Create a load operation with state edges
    static std::unique_ptr<llvm::LlvmRvsdgModule> CreateLoad(
        size_t numStates = 1);

    // Verify the load has correct memory dependencies
    static bool HasCorrectMemoryDeps(const rvsdg::Node* node);
};

class StorePattern {
public:
    // Create a store operation with state edges
    static std::unique_ptr<llvm::LlvmRvsdgModule> CreateStore(
        size_t numStates = 1);

    // Verify the store has correct memory dependencies
    static bool HasCorrectMemoryDeps(const rvsdg::Node* node);
};

}
```

### 4.3 Bus Protocol Patterns

```cpp
// patterns/BusProtocolPatterns.cpp
namespace jlm::hls::test {

class ReadyValidPattern {
public:
    // Create a ready/valid bundle type
    static std::shared_ptr<const BundleType> CreateReadyValid();

    // Verify the bundle has correct structure
    static bool IsValidBundle(const BundleType& type);
};

class BufferPattern {
public:
    // Create a buffer with specific capacity
    static std::unique_ptr<llvm::LlvmRvsdgModule> CreateBuffer(size_t capacity);

    // Verify the buffer generates correct FIRRTL
    static void VerifyFirrtlBuffer(
        const std::string& output,
        size_t expectedCapacity);
};

}
```

---

## H. Unit Test Examples

### 5.1 Operation Generator Tests

```cpp
// generators/ArithmeticOpGeneratorTests.cpp
#include <gtest/gtest.h>
#include "generators/ArithmeticOpGenerator.hpp"
#include "fixtures/BaseHlsTestFixture.hpp"

class ArithmeticOpGeneratorTest : public BaseHlsTestFixture {
protected:
    void SetUp() override {
        BaseHlsTestFixture::SetUp();
        generator_ = std::make_unique<ArithmeticOpGenerator>();
    }

    std::unique_ptr<ArithmeticOpGenerator> generator_;
};

TEST_F(ArithmeticOpGeneratorTest, CanHandleAddNode) {
    auto module = CreateSimpleAddModule();
    auto* lambda = GetHlsLambda(*module);
    auto* addNode = FindAddNode(lambda->subregion());

    EXPECT_TRUE(generator_->CanHandle(addNode));
}

TEST_F(ArithmeticOpGeneratorTest, GeneratesCorrectFIRRTLFor32BitAdd) {
    auto module = CreateSimpleAddModule();
    auto* lambda = GetHlsLambda(*module);
    auto* addNode = FindAddNode(lambda->subregion());

    RhlsToFirrtlConverter converter;
    auto fModule = generator_->Generate(converter, addNode);

    auto circuitText = converter.toString(fModule.getCircuit());

    // Verify FIRRTL output
    EXPECT_TRUE(ContainsFIRRTL(circuitText, "add"));
    EXPECT_TRUE(ContainsFIRRTL(circuitText, "UInt<32>"));
}
```

### 5.2 Transformation Tests

```cpp
// passes/GammaConversionPassTests.cpp
#include <gtest/gtest.h>
#include "passes/GammaConversionPass.hpp"
#include "fixtures/BaseHlsTestFixture.hpp"

class GammaConversionPassTest : public BaseHlsTestFixture {};

TEST_F(GammaConversionPassTest, ConvertsGammaToMux) {
    // Create module with gamma node
    auto module = CreateGammaModule();

    // Count gamma nodes before
    size_t gammaBefore = CountGammaNodes(*module);

    // Run transformation
    GammaNodeConversion pass;
    util::StatisticsCollector stats;
    pass.Run(module, stats);

    // Verify gamma replaced with mux
    size_t gammaAfter = CountGammaNodes(*module);
    size_t muxAfter = CountMuxNodes(*module);

    EXPECT_EQ(gammaAfter, 0);
    EXPECT_EQ(gammaBefore, muxAfter);
}

TEST_F(GammaConversionPassTest, HandlesNestedGammas) {
    auto module = CreateNestedGammaModule();

    GammaNodeConversion pass;
    util::StatisticsCollector stats;
    pass.Run(module, stats);

    // All gammas should be converted
    EXPECT_EQ(CountGammaNodes(*module), 0);
}
```

### 5.3 Integration Tests

```cpp
// integration/PipelineIntegrationTests.cpp
#include <gtest/gtest.h>
#include "rvsdg2rhls/rvsdg2rhls.hpp"
#include "rhls2firrtl/RhlsToFirrtlConverter.hpp"

class PipelineIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<llvm::LlvmRvsdgModule> CreateAndConvert(
        const std::string& llvmIr);
};

TEST_F(PipelineIntegrationTest, FullPipelineSimpleAdd) {
    auto module = CreateAndConvert(R"(
        %add = add i32 %a, %b
    )");

    // Convert to R-HLS
    rvsdg2rhls(*module);

    // Convert to FIRRTL
    RhlsToFirrtlConverter converter;
    auto firrtl = converter.ToString(*module);

    // Verify output contains expected patterns
    EXPECT_TRUE(ContainsFIRRTL(firrtl, "add"));
}

TEST_F(PipelineIntegrationTest, FullPipelineWithControlFlow) {
    auto module = CreateAndConvert(R"(
        %gamma = gamma (%cond) {
            %a = ...
        } else {
            %b = ...
        }
    )");

    rvsdg2rhls(*module);
    RhlsToFirrtlConverter converter;
    auto firrtl = converter.ToString(*module);

    // Verify when/else generated
    EXPECT_TRUE(ContainsFIRRTL(firrtl, "when"));
    EXPECT_TRUE(ContainsFIRRTL(firrtl, "else"));
}
```

---

## I. Test Execution Strategy

### Running Tests

```bash
# Run all HLS tests
make check

# Run specific test binary
./build/run-libhls-tests --gtest_brief=0

# Run specific test suite
./build/run-libhls-tests --gtest_filter=GammaConversion.*

# List all available tests
./build/run-libhls-tests --gtest_list_tests
```

### Test Categories

| Category | Pattern | Purpose |
|----------|---------|---------|
| Unit Tests | `*Tests.cpp` | Individual component tests |
| Integration Tests | `IntegrationTests.cpp` | Component interactions |
| Regression Tests | `RegressionTests.cpp` | Prevent future regressions |

---

## J. New Test Files to Create

### Phase 6 Test Plan
| File | Purpose | Est. Tests |
|------|---------|------------|
| `fixtures/BaseHlsTestFixture.hpp/cpp` | Base test setup | N/A |
| `fixtures/FirrtlVerifyingFixture.hpp/cpp` | FIRRTL verification | N/A |
| `patterns/ControlFlowPatterns.cpp` | Gamma/Theta patterns | 10+ |
| `patterns/MemoryAccessPatterns.cpp` | Load/store patterns | 8+ |
| `patterns/BusProtocolPatterns.cpp` | Ready/valid patterns | 6+ |
| `integration/PipelineIntegrationTests.cpp` | Full pipeline tests | 5+ |

### Test File Registration in Makefile.sub

```makefile
run-libhls-tests_SOURCES += \
    # Phase 6: New test infrastructure
    jlm/hls/backend/tests/fixtures/BaseHlsTestFixture.cpp \
    jlm/hls/backend/tests/fixtures/FirrtlVerifyingFixture.cpp \
    \
    # Phase 6: Test patterns
    jlm/hls/backend/tests/patterns/ControlFlowPatterns.cpp \
    jlm/hls/backend/tests/patterns/MemoryAccessPatterns.cpp \
    jlm/hls/backend/tests/patterns/BusProtocolPatterns.cpp \
    \
    # Phase 6: Integration tests
    jlm/hls/backend/tests/integration/PipelineIntegrationTests.cpp
```

---

## K. Code Coverage Requirements

### Minimum Coverage Targets

| Component | Target Coverage |
|-----------|-----------------|
| All HLS operations | > 90% |
| Transformation passes | > 85% |
| FIRRTL generation | > 80% |
| Integration tests | N/A (end-to-end) |

### Coverage Reporting

```bash
# Generate coverage report
make test_coverage

# View specific file coverage
gcov jlm/hls/backend/rhls2firrtl/RhlsToFirrtlConverter.cpp
```

---

## L. Best Practices for HLS Tests

### Test Naming Conventions
- Use `Test` suffix for test classes (e.g., `GammaConversionPassTest`)
- Use descriptive test names: `ConvertsGammaToMux`, `GeneratesCorrectFIRRTL`
- Prefix with what is being tested: `GammaNodeConversion_`, `MemoryOpGenerator_`

### Test Organization
- One test class per component/feature
- Group related tests together using Google Test's test case naming
- Use fixtures to reduce duplication (e.g., `BaseHlsTestFixture`)

### Test Isolation
- Each test should be independent
- Set up clean state for each test in `SetUp()`
- Clean up after each test in `TearDown()`

---

## M. Success Criteria

### Phase 6 Completion Checklist
- [ ] Test fixtures created and documented
- [ ] Test patterns implemented
- [ ] Integration tests added
- [ ] Code coverage tracked
- [ ] Regression tests defined
- [ ] External test suite analysis completed (see Appendix C)

---

*This phase ensures all HLS functionality is properly tested with a robust test infrastructure, following jlm's coding conventions. The external test suite analysis reveals gaps in unit testing that can be addressed without the full Verilator-based pipeline.*