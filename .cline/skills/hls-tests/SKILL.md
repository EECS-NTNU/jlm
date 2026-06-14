---
name: hls-tests
description: HLS unit testing framework for C++. Use when writing, running, or debugging HLS-related unit tests in the JLM compiler.
---

## See Also

- **HLS REFACTORING**: [HLS backend refactoring conventions](skill:hls-refactoring) - for refactoring code while maintaining compatibility with existing patterns

# HLS Unit Testing Framework

This skill covers writing and debugging C++ unit tests for the JLM HLS (High‑Level Synthesis) backend.

## When to Activate
- Writing new HLS unit tests  
- Debugging failing HLS tests (`make check` / `ctest`)  
- Working with RVSDG‑to‑FIRRTL conversion in tests  
- Fixing gamma node conversion issues  
- Testing HLS FIRRTL generators (`RhlsToFirrtlConverter`)

## Core Testing Concepts

### Gamma Node Conversion is Required
Tests that create gamma nodes **must** call `GammaNodeConversion::CreateAndRun()` before FIRRTL generation.

```cpp
// Arrange – create lambda with gamma node
auto controlType = jlm::rvsdg::ControlType::Create(2);
auto bitType    = jlm::rvsdg::BitType::Create(32);

jlm::llvm::LlvmRvsdgModule rm(jlm::util::FilePath(""), "", "");
auto lambdaNode = jlm::rvsdg::LambdaNode::Create(...);

// Create gamma node
auto gamma = GammaNode::create(lambdaNode->GetFunctionArguments()[0], 2);
// … add entry/exit vars …

jlm::util::StatisticsCollector stats;
GammaNodeConversion::CreateAndRun(rm, stats);   // <-- required

// Now generate FIRRTL
RhlsToFirrtlConverter conv;
auto firrtl = conv.ToString(rm);
```

### ControlType Direct Predicate
Use `ControlType` directly as the gamma predicate; avoid wrapping it in a `MatchOperation`.

## Typical Test Structure
```cpp
TEST(HlsTestSuite, Example) {
  using namespace jlm::hls;
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;

  // 1. Types
  auto ctl = ControlType::Create(2);
  auto bit = BitType::Create(32);

  // 2. Module & lambda
  LlvmRvsdgModule rm(FilePath(""), "", "");
  auto lambda = LambdaNode::Create(
      rm.Rvsdg().GetRootRegion(),
      LlvmLambdaOperation::Create(FunctionType::Create({ctl, bit}, {bit}),
                                 "f", Linkage::externalLinkage));

  // 3. Gamma node
  auto gamma = GammaNode::create(lambda->GetFunctionArguments()[0], 2);
  auto ev    = gamma->AddEntryVar(lambda->GetFunctionArguments()[1]);

  // 4. Exit vars
  std::vector<Output *> branchArgs;
  for (size_t i = 0; i < 2; ++i) branchArgs.push_back(ev.branchArgument[i]);
  auto ex = gamma->AddExitVar(branchArgs);

  // 5. Finalize lambda & export
  auto f = lambda->finalize({ex.output});
  GraphExport::Create(*f, "test");

  // 6. Run required conversion
  StatisticsCollector stats;
  GammaNodeConversion::CreateAndRun(rm, stats);

  // 7. Generate FIRRTL and assert
  RhlsToFirrtlConverter conv;
  auto firrtl = conv.ToString(rm);
  EXPECT_FALSE(firrtl.empty());
  EXPECT_TRUE(ContainsSubstring(firrtl, "when"));
}
```

## Running Tests
```bash
# All tests
make check

# HLS‑specific tests only
./build/run-libhls-tests

# Specific test suite
./build/run-libhls-tests --gtest_filter=RhlsToFirrtlConverterTestsControl.*

# Single test
./build/run-libhls-tests --gtest_filter=StreamConversionTests.*
```

## Common Error Messages and Fixes
| Error | Cause | Fix |
|-------|-------|-----|
| `Type error - expected : BitType, received : ctl(N)` | ControlType passed to `MatchOperation` | Pass `ControlType` directly as predicate |
| `Incorrect number of results` | Lambda finalize args mismatch | Ensure `finalize()` receives exactly the expected number of result values |
| `Incorrect number of values` | `AddExitVar` vector size mismatch | Vector length must equal `gamma->nsubregions()` |
| FIRRTL conversion fails (unconverted gamma) | Missing `GammaNodeConversion::CreateAndRun` call | Call it before FIRRTL generation |

## Quick Reference
| Command | Description |
|---------|-------------|
| `make check`                     | Run all unit tests |
| `make valgrind-check`            | Memory‑check tests with Valgrind |
| `./build/run-libhls-tests --gtest_filter=TestName.*` | Run a specific HLS test suite |

## FAQ
**Q:** Why does a test pass locally but fail on CI?  
**A:** CI may use a different compiler version or miss the CIRCT build. Reproduce the CI environment (`./configure.sh` with same flags) and ensure all dependencies (CIRCT, MLIR) are built.

**Q:** Missing `gtest` library?  
**A:** Install via package manager (`apt-get install libgtest-dev`) or build from source and point CMake to it.

## SEE ALSO
- **HLS REFACTORING**: [Backend refactoring conventions](skill:hls-refactoring)
- **HLS**: [High‑Level Synthesis mode](skill:hls)
- **TESTING**: [General testing framework](skill:testing)