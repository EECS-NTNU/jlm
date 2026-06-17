# Phase 3: rhls2firrtl Refactoring Plan

## Document Information
- **Phase**: 3 (rhls2firrtl)
- **Priority**: HIGH - Core conversion logic
- **Status**: Updated Plan

---

## 1. Current Architecture Analysis

### RhlsToFirrtlConverter.cpp Breakdown

```
jlm/hls/backend/rhls2firrtl/
├── RhlsToFirrtlConverter.hpp    # ~330 lines (public interface)
├── RhlsToFirrtlConverter.cpp    # 4126 lines (enormous!)
└── [supporting files]
```

### Key Issues Identified

1. **RhlsToFirrtlConverter.cpp is 4126 lines** - too large to maintain
2. **Giant switch statement pattern** - MlirGenSimpleNode handles all simple operations
3. **Mixed concerns** - FIRRTL generation, bus protocol handling, helper functions all mixed
4. **Hard to test** - No way to test individual operation generators in isolation

### Error Handling Strategy (Updated - June 14, 2026)

#### Original Approach (Identified Issues)
When an operation is not implemented, the original plan specified using `JLM_UNREACHABLE` with a descriptive message:

```cpp
// Use this for unimplemented operations:
JLM_UNREACHABLE("Operation type not yet implemented: " + node->DebugString());
```

This follows jlm's convention of using `JLM_UNREACHABLE` (from `jlm/util/common.hpp`) instead of exceptions.

#### Problems with Original Approach
Analysis identified the following issues with using `JLM_UNREACHABLE`:
- **Crashes the compiler** instead of graceful error handling
- **Poor user experience** when an operation is not yet implemented
- **No way to list missing operations** in a helpful error message

#### Recommended Approach (Updated)
1. Use `std::runtime_error` for missing generators during migration phase
2. Implement `GetSupportedOperations()` method for diagnostics:
   ```cpp
   class OperationGenerator {
   public:
       virtual std::vector<const char*> GetSupportedOperations() const = 0;
   };
   ```
3. Implement generator registry with helpful error messages:
   ```cpp
   class GeneratorRegistry {
   public:
       // Returns nullptr if no generator found for node type
       OperationGenerator* FindGenerator(const rvsdg::Node* node) const;

       // Throws std::runtime_error with list of supported operations
       OperationGenerator& RequireGenerator(const rvsdg::Node* node) const;
   };
   ```

#### Migration Strategy

The rhls2firrtl refactoring uses a **dual-implementation migration pattern**:

1. **Phase 3.1 - Generator Interface**: Create OperationGenerator base class with runtime_error
   - Define interface: `generate()`, `name()`, `GetSupportedOperations()`
   - Implement GeneratorRegistry with thread-safe lookup
   - Each generator throws std::runtime_error for unimplemented operations

2. **Phase 3.2-3.X - Incremental Migration**: Extract operations to generators one group at a time
   - Create wrapper generator that delegates to existing MlirGen* method
   - Add tests for the new generator
   - Update RhlsToFirrtlConverter.cpp to use registry for each operation type

3. **Phase 3.Final - Cleanup**: Remove old code after all generators are in place
   - Once all operations have generators, remove old MlirGen* methods from RhlsToFirrtlConverter.cpp
   - Switch any internal JLM_UNREACHABLE calls to proper error handling

#### Generator Registration Pattern

```cpp
// generators/register_all_generators.cpp
#include "GeneratorRegistry.hpp"
#include "ArithmeticOpGenerator.hpp"

void registerAllGenerators() {
  auto &reg = GeneratorRegistry::instance();
  reg.registerGenerator(rhls::Opcode::Add, 
                       std::make_unique<ArithmeticOpGenerator>());
}
```

#### Coexistence During Migration

During Phase 3, both patterns may coexist:

| Old Pattern (MlirGen*) | New Pattern (Generator) |
|------------------------|-------------------------|
| `RhlsToFirrtlConverter.cpp` large switch statement | `GeneratorRegistry` lookup |
| `JLM_UNREACHABLE("not implemented")` in converter | `throw runtime_error(...)` in generator |

**Migration Trigger**: Switch from MlirGen* to generators when:
1. A generator for that operation type has tests passing
2. The converter uses the registry instead of direct calls

#### Migration Checklist (Before Removing Old Code)

- [ ] All operations have registered generators
- [ ] Test suite passes with new generators only
- [ ] No runtime_error exceptions during full pipeline execution
- [ ] FIRRTL output matches baseline (see Phase 8 for verification)

### Operation Coverage in RhlsToFirrtlConverter.cpp

| Category | Operations | Lines (approx) |
|----------|------------|----------------|
| Arithmetic | Add, Sub, And, Or, Xor, Mul, Div, Shifts | ~400 lines |
| Comparison | Eq, Neq, Lt, Gt, Leq, Geq | ~150 lines |
| Memory | Mem, Load, Store, LocalMem | ~800 lines |
| Control | Fork, StateGate, Buffer, Trigger | ~600 lines |
| Structural | Sink, Print, ExtModule | ~200 lines |
| Helper Functions | GetConstant, Connect, etc. | ~2000 lines |

---

## 2. Refactoring Goals

### Phase 3.1: Operation Generator Pattern

#### Design Note: Generator vs Transformation Pattern

The generator pattern used here is **different** from the transformation pattern used in rvsdg2rhls:

| Pattern | Extends | Key Method | Purpose |
|---------|---------|------------|---------|
| Transformation | `rvsdg::Transformation` | `Run(RvsdgModule&, StatisticsCollector&)` | Module-level transformations (GammaConversion) |
| Generator | Custom base class | `Generate(converter, node)` | FIRRTL code generation |

**Example comparison:**

```cpp
// Transformation pattern (rvsdg2rhls):
class GammaNodeConversion final : public rvsdg::Transformation {
public:
    void Run(rvsdg::RvsdgModule & module, util::StatisticsCollector & statisticsCollector) override;
};

// Generator pattern (rhls2firrtl):
class OperationGenerator {
public:
    virtual circt::firrtl::FModuleOp Generate(
        RhlsToFirrtlConverter& converter,
        const rvsdg::Node* node) = 0;
};
```

#### Goal
Split RhlsToFirrtlConverter into operation-specific generators:
- Each generator handles one node type
- Clean interface for generator registration
- Testable in isolation

#### Error Handling Strategy (Updated - June 14, 2026)

**IMPORTANT**: The original plan specified using `JLM_UNREACHABLE` for unimplemented operations. However, this approach has been identified as problematic:

**Problems with JLM_UNREACHABLE:**
- Crashes the compiler instead of graceful error handling
- Poor user experience when an operation is not yet implemented
- No way to list missing operations in a helpful error message

**Recommended Approach:**
1. Use `std::runtime_error` for missing generators (non-fatal until migration complete)
2. Implement `GetSupportedOperations()` method for diagnostics:
   ```cpp
   class OperationGenerator {
   public:
       virtual std::vector<const char*> GetSupportedOperations() const = 0;
   };
   ```
3. Implement generator registry with helpful error messages:
   ```cpp
   class GeneratorRegistry {
   public:
       // Returns nullptr if no generator found for node type
       OperationGenerator* FindGenerator(const rvsdg::Node* node) const;

       // Throws std::runtime_error with list of supported operations
       OperationGenerator& RequireGenerator(const rvsdg::Node* node) const;
   };
   ```

**Migration Strategy:**
- During refactoring, both old MlirGen* methods and new generators coexist
- Use runtime_error during transition phase
- Once all operations have generators, switch to JLM_UNREACHABLE for true unimplemented cases

#### Best Practices
1. Each generator handles one node type
2. Clean interface for generator registration
3. Testable in isolation
4. Error handling should guide developers (not crash)

### Phase 3.2: Directory Structure

```
jlm/hls/backend/rhls2firrtl/
├── generators/                  # NEW: Operation-specific generators
│   ├── GeneratorInterface.hpp    # Base class for all generators
│   ├── GeneratorRegistry.hpp/cpp # Registry of all generators
│   ├── ArithmeticOpGenerator.hpp/cpp  # Add, Sub, And, Or, Xor, Mul, Div
│   ├── ComparisonOpGenerator.hpp/cpp  # Eq, Neq, Lt, Gt, Leq, Geq
│   ├── MemoryOpGenerator.hpp/cpp      # Mem, Load, Store, LocalMem
│   ├── ControlFlowOpGenerator.hpp/cpp # Fork, StateGate, Buffer, Trigger
│   └── StructuralOpGenerator.hpp/cpp  # Sink, Print, ExtModule
├── builder/                     # NEW: FIRRTL construction helpers
│   ├── FirrtlBuilder.hpp/cpp        # Common FIRRTL patterns
│   ├── RegisterBuilder.hpp/cpp      # Register generation
│   ├── WhenElseBuilder.hpp/cpp      # when/else statement helper
│   └── BundleBuilder.hpp/cpp        # Bundle type helpers
└── RhlsToFirrtlConverter.hpp/cpp  # Orchestrator (refactored)
```

### Phase 3.3: FIRRTL Builder Helpers

#### Goal
Extract common FIRRTL generation patterns:
- When/else statement construction
- Register creation with reset
- Bus protocol (ready/valid) handling
- Bundle type navigation

---

## 3. Detailed Design

### 3.1 Generator Interface

```cpp
// generators/GeneratorInterface.hpp
#include <jlm/hls/backend/rhls2firrtl/base-hls.hpp>
#include <vector>

class OperationGenerator {
public:
    virtual ~OperationGenerator() = default;

    // Check if this generator can handle the given node type
    virtual bool CanHandle(const rvsdg::Node* node) const = 0;

    // Get list of supported operation debug strings for diagnostics
    virtual std::vector<const char*> GetSupportedOperations() const = 0;

    // Generate FIRRTL module for the node
    // Throws std::runtime_error with helpful message if not supported
    virtual circt::firrtl::FModuleOp Generate(
        RhlsToFirrtlConverter& converter,
        const rvsdg::Node* node) = 0;
};
```

### 3.2 Operation-Specific Generators

```cpp
// generators/ArithmeticOpGenerator.hpp
class ArithmeticOpGenerator final : public OperationGenerator {
public:
    bool CanHandle(const rvsdg::Node* node) const override;
    circt::firrtl::FModuleOp Generate(
        RhlsToFirrtlConverter& converter,
        const rvsdg::Node* node) override;

private:
    circt::firrtl::FModuleOp HandleAdd(RhlsToFirrtlConverter&, const rvsdg::Node*);
    circt::firrtl::FModuleOp HandleSub(RhlsToFirrtlConverter&, const rvsdg::Node*);
    // ... more handlers
};
```

### 3.3 Generator Registry

```cpp
// generators/GeneratorRegistry.hpp
class GeneratorRegistry {
public:
    void Register(std::unique_ptr<OperationGenerator> generator);

    // Thread-safe: Returns nullptr if no generator found for node type
    OperationGenerator* FindGenerator(const rvsdg::Node* node) const;

    // Throws std::runtime_error with list of supported operations
    OperationGenerator& RequireGenerator(const rvsdg::Node* node) const;

    size_t GetRegisteredCount() const;

private:
    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<OperationGenerator>> generators_;
};
```

### 3.4 GeneratorFactory (Simplified Approach)

```cpp
// generators/GeneratorRegistry.hpp (simplified factory method)
class GeneratorRegistry {
public:
    // Returns nullptr if no generator found for node type
    OperationGenerator* FindGenerator(const rvsdg::Node* node) const;

    // Throws std::runtime_error with helpful message listing supported operations
    // Example: "No generator registered for HLS_BRANCH. Supported: Add, Sub, Mux"
    OperationGenerator& RequireGenerator(const rvsdg::Node* node) const;
};

### 3.5 FIRRTL Builder Classes

```cpp
// builder/FirrtlBuilder.hpp
class FirrtlBuilder {
public:
    // When/else statements
    mlir::Block* CreateWhenElse(
        mlir::Block* body,
        mlir::Value condition,
        bool hasElse);

    // Registers with reset
    circt::firrtl::RegResetOp CreateRegisterWithReset(
        mlir::Block* body,
        circt::firrtl::FIRRTLBaseType type,
        mlir::Value clock,
        mlir::Value reset,
        mlir::Value init);

    // Bundle navigation helpers
    mlir::Value GetSubfield(mlir::Block* body, mlir::Value bundle, const char* name);
    void Connect(mlir::Block* body, mlir::Value sink, mlir::Value source);
};
```

---

## 4. Implementation Tasks

### Local Self‑Contained Test Harness

To verify that changes do not break existing behavior without requiring an external CI system, the following local scripts are provided:

- `scripts/update_firrtl_golden.sh` – builds the project and regenerates golden FIRRTL files for all HLS converter unit tests.
- `scripts/run_hls_baseline.sh` – builds the project, runs each test binary, compares its FIRRTL output against the stored gold files, and exits with a non‑zero status on any mismatch.
- (Optional) `scripts/check_performance.sh` – measures compile time and binary size for a small benchmark and ensures they stay within a 5 % tolerance of the recorded baseline.

These scripts are invoked from the repository root:

```bash
chmod +x scripts/*.sh   # make executable once
./scripts/update_firrtl_golden.sh   # run only when intentional changes are made
./scripts/run_hls_baseline.sh       # run after each refactoring step to catch regressions
```

Running `run_hls_baseline.sh` will output either:

```
✅ All FIRRTL outputs match golden files.
```

or a diff for any failing test, causing the script to exit with status 1. This provides immediate feedback during development and can be incorporated into pre‑commit hooks.

The test harness ensures functional equivalence of the RHLS→FIRRTL conversion before and after generator integration.

### Task 3.1: Create generators/ Directory Structure

- [ ] Create `generators/` directory
- [ ] Implement `GeneratorInterface.hpp`
- [ ] Implement `GeneratorRegistry.hpp/cpp`
- [ ] Add tests for generator infrastructure

**Test Coverage**: Generator registration, operation matching, creation

### Task 3.2: Create builder/ Directory Structure

- [ ] Create `builder/` directory
- [ ] Implement `FirrtlBuilder.hpp/cpp`
- [ ] Implement `RegisterBuilder.hpp/cpp`
- [ ] Implement `WhenElseBuilder.hpp/cpp`
- [ ] Add tests for builder classes

**Test Coverage**: FIRRTL pattern generation, edge cases

### Task 3.3: Extract Arithmetic Operations

- [ ] Create `ArithmeticOpGenerator.hpp/cpp`
- [ ] Move Add/Sub logic to handlers
- [ ] Move And/Or/Xor logic to handlers
- [ ] Move Mul/Div logic to handlers
- [ ] Move shift operations to handlers
- [ ] Add comprehensive tests for each operation

**Error Handling**: Use `JLM_UNREACHABLE("Operation not implemented: " + node->DebugString())` for unimplemented cases

**Test Coverage**: All arithmetic operations with various bit widths

### Task 3.4: Extract Comparison Operations

- [ ] Create `ComparisonOpGenerator.hpp/cpp`
- [ ] Move comparison operations (Eq, Neq, Lt, Gt, etc.)
- [ ] Add tests for comparisons

**Error Handling**: Use `JLM_UNREACHABLE` for unsupported comparison types

### Task 3.5: Extract Memory Operations

- [ ] Create `MemoryOpGenerator.hpp/cpp`
- [ ] Move Mem/Load/Store logic
- [ ] Move LocalMem logic
- [ ] Move MemReq/MemResp logic
- [ ] Add tests for memory operations

**Test Coverage**: Load/store, read/write, different data widths

### Task 3.6: Extract Control Flow Operations

- [ ] Create `ControlFlowOpGenerator.hpp/cpp`
- [ ] Move Fork/StateGate logic
- [ ] Move Buffer/Trigger logic
- [ ] Add tests for control flow

**Test Coverage**: Fork (regular and constant), buffers with different capacities

### Task 3.7: Extract Structural Operations

- [ ] Create `StructuralOpGenerator.hpp/cpp`
- [ ] Move Sink/Print logic
- [ ] Move ExtModule logic
- [ ] Add tests for structural operations

### Task 3.8: Refactor RhlsToFirrtlConverter

```cpp
// Updated header
#include <stdexcept>

class RhlsToFirrtlConverter : public BaseHLS {
public:
    // ... existing constructor and public methods ...

private:
    std::unique_ptr<GeneratorRegistry> generatorRegistry_;
    std::unique_ptr<FirrtlBuilder> firrtlBuilder_;

    circt::firrtl::FModuleOp GenerateNode(const rvsdg::Node* node);
};

// Updated implementation with improved error handling
circt::firrtl::FModuleOp RhlsToFirrtlConverter::GenerateNode(
    const rvsdg::Node* node)
{
    auto* generator = generatorRegistry_->FindGenerator(node);
    if (!generator) {
        // Get list of supported operations for helpful error message
        std::vector<const char*> supportedOps;
        size_t registeredCount = generatorRegistry_->GetRegisteredCount();
        
        // Build error message with available generators
        std::string errorMsg = "No generator registered for: ";
        errorMsg += node->debug_string();
        errorMsg += ". Available generators: ";
        
        if (registeredCount == 0) {
            errorMsg += "(none)";
        } else {
            errorMsg += std::to_string(registeredCount);
            errorMsg += " generators registered";
        }
        
        throw std::runtime_error(errorMsg);
    }
    return generator->Generate(*this, node);
}
```

### Task 3.9: Update Makefile.sub

Register new files:
```makefile
run-libhls-tests_SOURCES += \
    jlm/hls/backend/rhls2firrtl/generators/GeneratorRegistryTests.cpp \
    jlm/hls/backend/rhls2firrtl/generators/ArithmeticOpGeneratorTests.cpp \
    # ... add all new generator test files
```

---

## 5. Test Strategy for Phase 3

### Unit Tests for Generators

```cpp
// generators/ArithmeticOpGeneratorTests.cpp
#include <gtest/gtest.h>
#include "generators/ArithmeticOpGenerator.hpp"

TEST(ArithmeticOpGenerator, CanHandleAddNode) {
    auto module = CreateTestModule();
    auto* addNode = AddAddOperation(module);

    ArithmeticOpGenerator generator;
    EXPECT_TRUE(generator.CanHandle(addNode));
}

TEST(ArithmeticOpGenerator, GeneratesCorrectFIRRTLFor32BitAdd) {
    auto module = CreateTestModule();
    auto* addNode = AddAddOperation(module, 32, 10, 20);

    RhlsToFirrtlConverter converter;
    auto fModule = generator_->Generate(converter, addNode);

    auto circuitText = converter.toString(fModule.getCircuit());

    EXPECT_TRUE(ContainsFIRRTL(circuitText, "add"));
    EXPECT_TRUE(ContainsFIRRTL(circuitText, "UInt<32>"));
}
```

### Integration Tests

```cpp
// generators/GeneratorRegistryTests.cpp
TEST(GeneratorRegistry, ThrowsForUnimplementedOperation) {
    auto module = CreateTestModule();
    auto* unsupportedNode = AddUnsupportedOperation(module);

    RhlsToFirrtlConverter converter;
    EXPECT_DEATH(converter.GenerateNode(unsupportedNode), "No generator registered");
}
```

### Regression Tests

```bash
# Run all rhls2firrtl tests
./build/run-libhls-tests --gtest_filter=RHLS2FIRRTL.*

# Run specific generator test
./build/run-libhls-tests --gtest_filter=*ArithmeticOpGenerator*

# Verify full pipeline still works
./build/run-libhls-tests --gtest_filter=FullPipelineTest.*
```

---

## 6. Files to Create/Modify

### New Files to Create
| File | Purpose |
|------|---------|
| `generators/GeneratorInterface.hpp` | Generator base class |
| `generators/GeneratorRegistry.hpp/cpp` | Registry implementation |
| `builder/FirrtlBuilder.hpp/cpp` | Common FIRRTL patterns |
| `builder/RegisterBuilder.hpp/cpp` | Register helpers |
| `builder/WhenElseBuilder.hpp/cpp` | When/else helper |
| `builder/BundleBuilder.hpp/cpp` | Bundle helpers |
| `generators/*OpGenerator.hpp/cpp` | Operation generators (5 files) |

### Existing Files to Modify
| File | Change |
|------|--------|
| `RhlsToFirrtlConverter.hpp` | Add generator registry, builder members |
| `RhlsToFirrtlConverter.cpp` | Use generators instead of switch statements |

---

## 7. Success Criteria

### Phase 3 Completion Checklist
- [ ] Generator interface and registry implemented
- [ ] All operations extracted to generators
- [ ] Builder classes created for common patterns
- [ ] RhlsToFirrtlConverter refactored to use generators
- [ ] Makefile.sub updated with new files
- [ ] All existing tests still pass
- [ ] New unit tests for each generator

### Quality Metrics
| Metric | Target |
|--------|--------|
| Lines per generator file | < 300 lines |
| Generators testable in isolation | Yes |
| Test coverage per generator | > 80% |

---

## 8. Pull Request Strategy

### Critical PR Splitting for Phase 3

The `RhlsToFirrtlConverter.cpp` (4126 lines) must be split across **multiple focused PRs**:

| PR # | Operation Category | Lines Moved | Tests Required |
|------|-------------------|-------------|----------------|
| 1 | Generator interface + registry | ~300 | Integration tests |
| 2 | Arithmetic ops (Add, Sub, And, Or) | ~400 | Unit tests per op |
| 3 | Arithmetic ops (Xor, Mul, Div, Shifts) | ~400 | Unit tests per op |
| 4 | Comparison ops (Eq, Neq, Lt, Gt, Leq, Geq) | ~150 | Unit tests |
| 5 | Memory ops (Mem, Load, Store, LocalMem) | ~800 | Integration tests |
| 6 | Control flow (Fork, Buffer, Trigger) | ~600 | Integration tests |
| 7 | Structural (Sink, Print, ExtModule) | ~200 | Unit tests |
| 8 | Refactor orchestrator to use generators | ~500 | Full pipeline test |

**Total**: 8 PRs for Phase 3 refactoring

### Each Generator PR Must Include
- [ ] New generator class (canHandle(), generate() methods)
- [ ] Extracted operation handling logic
- [ ] Unit tests for each operation
- [ ] Registration in generator registry
- [ ] Update RhlsToFirrtlConverter to use generators

### Example PR Title (Generator PR)
```
hls/rhls2firrtl: Create ArithmeticOpGenerator for Add, Sub, And, Or

- Implement OperationGenerator interface (canHandle, generate methods)
- Extract Add/Sub/Xor/Mul logic from RhlsToFirrtlConverter.cpp
- Add unit tests for 32-bit and 64-bit variants
- Register in GeneratorRegistry

Refactoring Type: Generator pattern extraction
PR Type: refactor + test
```

### Example PR Title (Orchestrator PR)
```
hls/rhls2firrtl: Refactor RhlsToFirrtlConverter to use generators

- Replace giant switch statement with generator registry lookup
- Remove extracted logic from converter
- Add full pipeline integration tests
- Verify no functional changes in FIRRTL output

Refactoring Type: Code refactor
PR Type: refactor + test
```

### Pre-Submit Checklist (from 07_PR_GUIDELINES.md)
```bash
# 1. Build verification
make clean && make -j$(nproc) 2>&1 | grep -i "warning" > /tmp/warnings.txt

# 2. Test all existing functionality
./build/run-libhls-tests --gtest_filter=RHLS2FIRRTL.*

# 3. Code quality
clang-format jlm/hls/backend/rhls2firrtl/generators/*.hpp jlm/hls/backend/rhls2firrtl/generators/*.cpp

# 4. Verify no functional changes (output should match baseline)
./build/run-libhls-tests --gtest_filter=FullPipelineTest.*
```

---

## 9. Next Steps After Phase 3

1. Update documentation for new architecture
2. Add performance benchmarks
3. Consider additional optimizations (see Phase 4)

---

*This phase splits the massive RhlsToFirrtlConverter into manageable, testable operation generators.*

#### Error Handling Strategy (Updated - June 14, 2026)
- Use `std::runtime_error` with helpful messages during migration
- Implement `GetSupportedOperations()` method for diagnostics
- Thread-safe generator registry with mutex protection
- Once all operations have generators, switch to JLM_UNREACHABLE for true unimplemented cases

#### Migration Path
1. Implement OperationGenerator base class (Phase 0)
2. Create GeneratorRegistry with thread-safe registration
3. Extract operations one at a time to generators
4. Use runtime_error for missing generators during transition
5. Finalize by removing old MlirGen* methods from converter
