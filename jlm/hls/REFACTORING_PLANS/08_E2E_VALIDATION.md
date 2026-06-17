# Phase 8: End-to-End Validation (Ongoing)

## Document Information
- **Phase**: 8 (E2E Validation)
- **Priority**: HIGH - Quality assurance
- **Status**: Planning Phase
- **Last Updated**: June 14, 2026

---

## 1. E2E Validation Requirements Added - June 14, 2026

This phase addresses missing end-to-end validation strategy identified in refactoring analysis:

### Missing Components from Original Phase 6
| Component | Status |
|-----------|--------|
| FIRRTL → Verilog lowering tests | Missing |
| Output equivalence verification | Missing |
| Hardware-level validation | Missing |

### New E2E Validation Goals
1. Verify FIRRTL output can be lowered to valid Verilog using CIRCT
2. Ensure refactoring doesn't change hardware semantics (bit-identical)
3. Detect performance regressions in generated code

---

## 2. FIRRTL Verification Strategy

### 2.1 FIRRTL → Verilog Lowering Tests

**Tools**: Use CIRCT's `firtool` for FIRRTL to Verilog lowering

```bash
# Generate FIRRTL from HLS pipeline
./build/run-libhls-tests --gtest_filter=PipelineTest.SimpleAdd > /tmp/output.fir

# Lower to Verilog using CIRCT
firtool -oVerilog /tmp/output.fir > /tmp/output.v

# Verify Verilog is valid (has module declaration)
grep "module" /tmp/output.v | head -1
```

### 2.2 Roundtrip Verification Tests

```cpp
// integration/RoundtripTests.cpp
#include <gtest/gtest.h>

class FIRRTLRoundtripTest : public ::testing::Test {
protected:
    // Convert HLS -> FIRRTL -> Verilog and back
    std::string FullPipeline(const std::string& llvmIr);
};

TEST_F(FIRRTLRoundtripTest, SimpleAdd) {
    auto llvmIr = R"(
        %add = add i32 %a, %b
    )";
    
    // Run through full pipeline
    auto verilog = FullPipeline(llvmIr);
    
    // Verify Verilog contains expected operations
    EXPECT_TRUE(ContainsSubstring(verilog, "add"));
}

TEST_F(FIRRTLRoundtripTest, ControlFlow) {
    auto llvmIr = R"(
        %result = select %cond, %a, %b
    )";
    
    auto verilog = FullPipeline(llvmIr);
    
    // Verify Verilog contains conditional logic
    EXPECT_TRUE(ContainsSubstring(verilog, "if") || ContainsSubstring(verilog, "?"));
}
```

### 2.3 Output Equivalence Tests

**Goal**: Ensure refactored code generates semantically equivalent FIRRTL to original

#### FIRRTL Normalization Strategy

FIRRTL output may differ syntactically (whitespace, comments, ordering) while being semantically identical. The normalization strategy:

| Aspect | Should Match | Rationale |
|--------|--------------|-----------|
| Operation names and types | Yes | Semantic equivalence |
| Bit widths | Yes | Hardware semantics |
| Module structure | Yes | Top-level module hierarchy |
| Register names (without prefix) | No | Internal naming can vary |
| Whitespace/comments | N/A | Not semantically meaningful |

**Normalization Function**:
```cpp
// Normalize FIRRTL for comparison
std::string NormalizeFIRRTL(const std::string& firrtl) {
    // 1. Remove comments and empty lines
    // 2. Canonicalize whitespace (single spaces, no trailing)
    // 3. Sort operation definitions alphabetically by module
    // 4. Strip line numbers from debug info
    
    std::regex commentRegex("//[^\\n]*");
    std::string normalized = std::regex_replace(firrtl, commentRegex, "");
    
    // Further normalization as needed...
    return normalized;
}
```

#### Semantic Equivalence Verification

**Important**: FIRRTL is **not deterministic** for certain operations:
- Commutative operations: `add(a, b)` == `add(b, a)`
- Memory addressing order may vary
- Internal wire naming can differ

**Recommended Approach**:

1. **String comparison with normalization** (fast, good for basic changes)
   ```cpp
   EXPECT_EQ(NormalizeFIRRTL(reference), NormalizeFIRRTL(current));
   ```

2. **AST comparison** (more robust, requires FIRRTL parser)
   ```cpp
   // Parse both FIRRTL to MLIR AST
   auto ast1 = parseFIRRTL(reference);
   auto ast2 = parseFIRRTL(current);
   
   // Compare AST structure (ignoring location info, names)
   EXPECT_TRUE(AstEqual(ast1, ast2));
   ```

3. **Circuit equivalence** (gold standard, via CIRCT's equivalences)
   ```cpp
   // Use CIRCT's equivalence checking passes
   auto circtCtx = createCIRCTContext();
   bool equivalent = checkCircuitEquivalence(firrtl1, firrtl2);
   EXPECT_TRUE(equivalent);
   ```

#### Test Examples

```cpp
TEST_F(EquivalenceTest, AdditionGeneratesSameFIRRTL) {
    auto module = CreateAddTestModule();
    auto* addNode = FindAddNode(module);
    
    auto reference = GetReferenceOutput(addNode);
    auto current = GetCurrentOutput(addNode);
    
    // FIRRTL should be semantically equivalent after normalization
    EXPECT_EQ(NormalizeFIRRTL(reference), NormalizeFIRRTL(current));
}

TEST_F(EquivalenceTest, CommutativeOperationsMatch) {
    // a + b and b + a should produce equivalent circuits
    auto module1 = CreateAddModule(a, b);
    auto module2 = CreateAddModule(b, a);
    
    auto output1 = GenerateFIRRTL(module1);
    auto output2 = GenerateFIRRTL(module2);
    
    EXPECT_EQ(NormalizeFIRRTL(output1), NormalizeFIRRTL(output2));
}

TEST_F(EquivalenceTest, GammaConversion) {
    auto module = CreateGammaTestModule();
    
    auto reference = GetReferenceOutput(module);
    auto current = GetCurrentOutput(module);
    
    // Both should generate same mux-based FIRRTL
    EXPECT_EQ(NormalizeFIRRTL(reference), NormalizeFIRRTL(current));
}
```

#### Baseline Storage

Store baseline FIRRTL outputs for comparison:

```bash
# Generate and store baseline (pre-refactor)
./scripts/verify-baseline.sh --store-firrtl /tmp/baseline-firrtl/

# Compare against baseline after refactoring
./scripts/run-hls-baseline.sh --compare /tmp/baseline-firrtl/
```

#### When Equivalence May Differ (Accepted)

| Scenario | Expected Behavior |
|----------|-------------------|
| Generator reordering operations | Normalization must handle |
| New helper functions added | May change wire names but not semantics |
| Internal register naming changes | Acceptable if functionally identical |

---

### 2.4 Formal Equivalence Verification (Optional - Advanced)

For critical operations, consider using formal equivalence checking:

```bash
# Use Yosys for formal equivalence checking
yosys -p "read_firrtl reference.fir; circuit_to_equivalent; read_firrtl current.fir; equivalence_check" 

# Or use CIRCT's equivalence passes
circt-opt --equivalence-checking reference.fir current.fir
```

```cpp
// integration/EquivalenceTests.cpp
#include <gtest/gtest.h>

class EquivalenceTest : public ::testing::Test {
protected:
    // Get reference output from unrefactored version
    std::string GetReferenceOutput(const rvsdg::Node* node);
    
    // Get current output (may be refactored)
    std::string GetCurrentOutput(const rvsdg::Node* node);
};

TEST_F(EquivalenceTest, AdditionGeneratesSameFIRRTL) {
    auto module = CreateAddTestModule();
    auto* addNode = FindAddNode(module);
    
    auto reference = GetReferenceOutput(addNode);
    auto current = GetCurrentOutput(addNode);
    
    // FIRRTL should be identical (ignoring whitespace/comments)
    EXPECT_EQ(NormalizeFIRRTL(reference), NormalizeFIRRTL(current));
}

TEST_F(EquivalenceTest, GammaConversion) {
    auto module = CreateGammaTestModule();
    
    auto reference = GetReferenceOutput(module);
    auto current = GetCurrentOutput(module);
    
    // Both should generate same mux-based FIRRTL
    EXPECT_EQ(NormalizeFIRRTL(reference), NormalizeFIRRTL(current));
}
```

---

## 3. Hardware Validation Strategy

### 3.1 Verilator Simulation Tests

**Goal**: Run generated Verilog through Verilator for functional validation

```cpp
// integration/VerilatorTests.cpp
#include <gtest/gtest.h>

class VerilatorTest : public ::testing::Test {
protected:
    // Generate and compile Verilog with Verilator
    std::string CompileAndRun(const std::string& verilog);
    
    // Verify simulation output matches expected values
    bool VerifySimulationOutput(
        const std::string& output,
        const std::vector<int>& expectedValues);
};

TEST_F(VerilatorTest, SimpleAdditionWorks) {
    auto llvmIr = R"(
        %add = add i32 5, 7
    )";
    
    // Generate Verilog
    auto verilog = GenerateVerilog(llvmIr);
    
    // Compile with Verilator and run simulation
    auto output = CompileAndRun(verilog);
    
    // Verify result is 12 (5 + 7)
    EXPECT_TRUE(VerifySimulationOutput(output, {12}));
}

TEST_F(VerilatorTest, LoopCount) {
    auto llvmIr = R"(
        %result = loop_count 3
    )";
    
    auto verilog = GenerateVerilog(llvmIr);
    auto output = CompileAndRun(verilog);
    
    // Verify loop executed 3 times
    EXPECT_TRUE(VerifySimulationOutput(output, {3}));
}
```

### 3.2 Test Cases for Hardware Validation

| Test Case | Description | Expected Behavior |
|-----------|-------------|-------------------|
| SimpleAdd | Basic arithmetic operation | Output equals sum |
| ControlFlow | Gamma/Theta to when/else | Correct conditional execution |
| MemoryAccess | Load/store operations | Data integrity maintained |
| LoopCarried | Theta with loop-carried values | Values propagated correctly |

---

## 4. Performance Validation Strategy

### 4.1 Benchmark Suite

**Goal**: Detect performance regressions in generated code

```bash
# Run benchmarks before and after refactoring
./build/run-benchmarks.sh --reference /tmp/before-refactor
```

### 4.2 Metrics to Track

| Metric | Description | Target |
|--------|-------------|--------|
| FIRRTL file size | Output size (bytes) | <10% increase |
| Verilog file size | Final output size | <10% increase |
| FIRRTL parsing time | Time to parse FIRRTL | <20% change |
| Compilation time | Full pipeline duration | <30% change |

### 4.3 Regression Detection

```cpp
// integration/PerformanceTests.cpp
#include <gtest/gtest.h>
#include <chrono>

class PerformanceTest : public ::testing::Test {
protected:
    template<typename Func>
    std::chrono::milliseconds Measure(Func func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start);
    }
};

TEST_F(PerformanceTest, PipelineDoesNotRegress) {
    auto module = CreateLargeTestModule();
    
    // Measure current performance
    auto duration = Measure([&]() {
        GenerateVerilog(module);
    });
    
    // Should not be more than 50% slower than baseline
    EXPECT_LT(duration.count(), 1.5 * baselineDuration_.count());
}
```

---

## 5. CI/CD Integration

### 5.1 Pre-Submit Checks

```bash
# Add to PR checklist
./scripts/e2e-validate.sh --input /tmp/test-module.fir
```

### 5.2 Automated Validation Pipeline

```
Refactored Code → Generate FIRRTL → Lower to Verilog → Run Tests
                                      ↓
                              Verify Size < 10%
                                      ↓
                              Compile with Verilator
                                      ↓
                              Simulate and Compare
```

---

## 6. Test File Structure

```
jlm/hls/backend/tests/
├── e2e/                               # NEW: E2E validation tests
│   ├── firrtl/                        # FIRRTL-specific tests
│   │   ├── FirrtlRoundtripTests.cpp
│   │   └── EquivalenceTests.cpp
│   ├── hardware/                      # Hardware validation tests
│   │   ├── VerilatorTests.cpp
│   │   └── SimulationTests.cpp
│   └── performance/                   # Performance regression tests
│       ├── PipelinePerformanceTests.cpp
│       └── SizeRegressionTests.cpp
```

---

## 7. Migration Path

### Phase 8.1: FIRRTL Validation Framework (Week 1)
- [ ] Implement roundtrip verification framework
- [ ] Create equivalence test infrastructure
- [ ] Set up CIRCT toolchain integration

### Phase 8.2: Hardware Validation (Week 2)
- [ ] Implement Verilator integration tests
- [ ] Create simulation result comparison utilities
- [ ] Add regression detection for common patterns

### Phase 8.3: Performance Benchmarking (Week 3)
- [ ] Establish performance baseline
- [ ] Implement regression detection
- [ ] Set up CI monitoring

---

## 8. Success Criteria

### Phase 8 Completion Checklist
- [ ] FIRRTL → Verilog lowering tests pass for all operations
- [ ] Output equivalence tests detect all functional changes
- [ ] Hardware validation catches semantics changes
- [ ] Performance benchmarks establish regression baseline

### Quality Metrics
| Metric | Target |
|--------|--------|
| E2E test coverage | >95% of operations |
| Regression detection rate | 100% for semantic changes |
| Performance monitoring latency | <1 hour |

---

*This phase ensures all HLS functionality is validated end-to-end, from HLS IR to hardware-ready Verilog.*

Version 1.0: Added June 14, 2026 as Phase 8 of refactoring plan.