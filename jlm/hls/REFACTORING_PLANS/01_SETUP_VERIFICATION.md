# Phase 1: Test Infrastructure & Verification Plan

## Document Information
- **Phase**: 1 (Setup)
- **Priority**: CRITICAL - Must complete before any refactoring
- **Estimated Duration**: 3-5 days
- **Status**: In Progress

---

## 1. Pre-Refactoring Verification Checklist

### Build System Verification
```bash
# Run all tests to establish baseline
make check 2>&1 | tee /tmp/pre-refactor-test-results.txt

# Verify HLS-specific tests
./build/run-libhls-tests --gtest_brief=0 2>&1 | tee /tmp/hls-tests-before.txt

# Check for compilation warnings
make clean && make -j$(nproc) 2>&1 | grep -i "warning" > /tmp/warnings-before.txt
```

### Test Coverage Metrics
```bash
# Generate coverage report (if gcov available)
cd build
cmake -DCOVERAGE=ON ..
make
make test_coverage 2>&1 | tee ../coverage-report.txt
```

---

## 2. Current Test Coverage Analysis

### Existing Test Files
| File | Tests | Status | Notes |
|------|-------|--------|-------|
| BaseHlsTests.cpp | 24 | ✅ Passing | Base HLS utilities |
| RhlsToFirrtlConverterTests.cpp | 15 | ✅ Passing | Basic FIRRTL generation |
| RhlsToFirrtlConverterTests_Control.cpp | 6 | ⚠️ Partial | Control flow tests |
| RhlsToFirrtlConverterTests_Memory.cpp | 7 | ❌ Linker issue | Memory tests pending |

### rvsdg2rhls Tests
| Test File | Status | Coverage Gap |
|-----------|--------|--------------|
| DeadNodeEliminationTests.cpp | ✅ | DNE doesn't remove all dead edges |
| DistributeConstantsTests.cpp | ⚠️ | Needs verification |
| ForkTests.cpp | ⚠️ | Constant fork tests needed |
| StreamConversionTests.cpp | ⚠️ | Partial coverage |
| GammaTests.cpp | ⚠️ | Mixed with other tests |
| MemoryConverterTests.cpp | ⚠️ | State splitting needed |
| ThetaTests.cpp | ⚠️ | Loop-carried values needed |
| UnusedStateRemovalTests.cpp | ✅ | Lambda/Gamma/Theta coverage |

### Test Files NOT Registered in Makefile.sub
| File | Status | Priority |
|------|--------|----------|
| GammaConversionTests.cpp | ❌ Missing file | HIGH |
| ThetaConversionTests.cpp | ❌ Missing file | HIGH |
| MemoryStateSplitConversionTests.cpp | ⚠️ Mixed with others | MEDIUM |

---

## 3. Pre-Refactoring Test Requirements

### Must-Have Tests Before Starting Refactoring
- [ ] All existing tests pass
- [ ] Test coverage baseline recorded
- [ ] Performance metrics captured
- [ ] Build warnings documented

### Test Execution Commands
```bash
# Run all HLS tests
make check 2>&1 | tee pre-refactor-all-tests.txt

# Run specific test binary
./build/run-libhls-tests --gtest_filter=*Gamma*:*Theta*

# Run with verbose output for debugging
./build/run-libhls-tests --gtest_brief=0 --gtest_list_tests

# Check for leaks (if valgrind available)
valgrind --leak-check=full ./build/run-libhls-tests 2>&1 | tee memory-check.txt
```

---

## 4. Test Infrastructure Setup

### New Test Directory Structure
```
jlm/hls/backend/tests/
├── fixtures/                    # Common test fixtures
│   ├── BaseHlsTestFixture.hpp/cpp
│   ├── FirrtlVerifyingFixture.hpp/cpp
│   └── RvsdgFixture.hpp/cpp
├── patterns/                    # Reusable test patterns
│   ├── ControlFlowPatterns.cpp
│   ├── MemoryAccessPatterns.cpp
│   └── BusProtocolPatterns.cpp
└── integration/                 # End-to-end tests
    ├── PipelineIntegrationTests.cpp
    └── RegressionTests.cpp
```

### Test Fixture Design

```cpp
// fixtures/BaseHlsTestFixture.hpp
class BaseHlsTestFixture : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;

  // Helper to create simple test module
  std::unique_ptr<RvsdgModule> CreateSimpleModule(
      const std::vector<Type*>& inputs,
      const std::vector<Type*>& outputs);

  // Helper to verify FIRRTL output matches expected pattern
  bool VerifyFirrtlPattern(const std::string& output, 
                          const std::regex& pattern);
};
```

---

## 5. Regression Test Suite

### Test Categories
1. **Unit Tests** - Individual functions/classes
2. **Integration Tests** - Transformation pipeline stages
3. **End-to-End Tests** - Full RVSDG → Verilog flow

### Regression Test Checklist
```bash
# Run regression tests after each refactoring phase
./build/run-libhls-tests \
  --gtest_filter=RVSDG2RHLS.*:RHLS2FIRRTL.* \
  --gtest_brief=1
```

---

## 6. Verification Script

Create `scripts/verify-refactoring.sh`:

```bash
#!/bin/bash
set -e

echo "=== Pre-Refactoring Verification ==="

# 1. Build verification
echo "Building project..."
make clean && make -j$(nproc)

# 2. Run all HLS tests
echo "Running HLS tests..."
./build/run-libhls-tests --gtest_brief=0 | tee /tmp/hls-test-results.txt

# 3. Check test count
TEST_COUNT=$(grep -c "PASSED" /tmp/hls-test-results.txt)
echo "Total passing tests: $TEST_COUNT"

# 4. Generate coverage report (if available)
if command -v gcov &> /dev/null; then
    echo "Generating coverage..."
    make test_coverage 2>&1 | tee coverage-report.txt
fi

echo "=== Verification Complete ==="
echo "Results saved to /tmp/hls-test-results.txt"
```

---

## 7. Rollback Point Definition

### Pre-Refactoring State Snapshot
Before starting any refactoring, create a git branch:

```bash
# Create rollback point
git checkout -b pre-refactor-state-base
git add .
git commit -m "Pre-refactoring state snapshot (June 13, 2026)"

# Save test results as reference
cp /tmp/hls-test-results.txt git_refs/
```

### Verification Before Each Phase
```bash
# After each phase, verify:
./scripts/verify-phase.sh
```

---

## 8. Success Criteria for Phase 1

- [ ] All existing tests pass (baseline recorded)
- [ ] Test infrastructure created and documented
- [ ] Regression test suite ready
- [ ] Verification scripts operational
- [ ] Rollback point established in git

### Documentation Deliverables
1. `REFACTORING_PLANS/01_SETUP_VERIFICATION.md` - This file (updated with results)
2. `/tmp/pre-refactor-test-results.txt` - Baseline test output
3. `scripts/verify-refactoring.sh` - Verification script
4. `docs/TESTING_STRATEGY.md` - Updated testing documentation

---

## 9. Next Steps After Phase 1 Completion

1. Review test coverage gaps
2. Update plan with discovered issues
3. Begin rvsdg2rhls refactoring (Phase 2)
4. Continue through all phases

---

*This phase must be completed before any code changes are made to the HLS backend.*