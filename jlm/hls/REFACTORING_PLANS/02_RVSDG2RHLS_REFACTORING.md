# Phase 2: rvsdg2rhls Refactoring Plan

## Document Information
- **Phase**: 2 (rvsdg2rhls)
- **Priority**: HIGH - Foundation layer
- **Status**: Updated Plan (June 14, 2026)

#### Dependencies Added - June 14, 2026
**Phase 0 Required First:**
- Phase 0.1 must complete before Phase 2 can begin:
  - OperationGenerator base class implementation
  - GeneratorRegistry infrastructure with thread-safe registration

---

## 1. Current Architecture Analysis

---

## 1. Current Architecture Analysis

### rvsdg2rhls Component Breakdown

```
jlm/hls/backend/rvsdg2rhls/
├── rvsdg2rhls.cpp              # Main orchestrator (479 lines)
├── rvsdg2rhls.hpp              # Public API
└── [individual transformations]
    ├── add-buffers.hpp/cpp
    ├── add-forks.hpp/cpp
    ├── GammaConversion.hpp/cpp
    └── ...
```

### Key Issues Identified

1. **rvsdg2rhls.cpp** is 479 lines - orchestrates many transformations but lacks structure
2. **No organized structure** - transformation files not grouped by purpose
3. **Mixed concerns** - optimization and conversion logic combined
4. **Limited testability** - hard to test individual transformations in isolation

---

## 2. Refactoring Goals

### Phase 2.1: Transformation Organization

#### Goal
Organize existing transformations into logical groups while preserving the existing `rvsdg::Transformation` interface:

- Keep all existing transformation classes
- Group files by purpose (optimization, conversion, verification)
- Create clear directory structure
- Maintain compatibility with existing codebase

### Phase 2.2: Directory Structure

#### New Organization

```
jlm/hls/backend/rvsdg2rhls/
├── passes/                     # NEW: Transformation pipeline classes
│   ├── optimization/           # Optimizations (DNE, CNE, etc.)
│   │   └── ...
│   ├── conversion/             # RVSDG → R-HLS conversions
│   │   ├── GammaConversion.hpp/cpp
│   │   ├── ThetaConversion.hpp/cpp
│   │   ├── MemoryStateSplit.hpp/cpp
│   │   └── StreamConversion.hpp/cpp
│   ├── buffering/              # Buffer/Fork/Sink insertion
│   │   ├── AddBuffersPass.hpp/cpp
│   │   ├── AddForksPass.hpp/cpp
│   │   ├── AddSinksPass.hpp/cpp
│   │   └── AddPrintsPass.hpp/cpp
│   └── verification/           # R-HLS verification passes
│       ├── CheckRhls.hpp/cpp
│       └── ...
└── rvsdg2rhls.cpp              # Updated to use transformation sequence
```

#### Transformation Interface (jlm Convention)

All transformations extend `rvsdg::Transformation` and implement the `Run()` method:

```cpp
class GammaNodeConversion final : public rvsdg::Transformation {
public:
    ~GammaNodeConversion() noexcept override;

    void Run(rvsdg::RvsdgModule & module, util::StatisticsCollector & statisticsCollector) override;
};
```

**Key points about the jlm transformation interface:**
- The `Run()` method is called multiple times; implementations must reset internal state between calls
- Use `util::StatisticsCollector` for collecting pass statistics (see example below)
- Follow the naming convention: `ClassName(final)` extending `rvsdg::Transformation`

**Example implementation pattern** (based on existing code):
```cpp
class BufferInsertion final : public rvsdg::Transformation {
public:
    ~BufferInsertion() noexcept override;
    
    void Run(rvsdg::RvsdgModule & module, util::StatisticsCollector & statisticsCollector) override;

    static void CreateAndRun(
        rvsdg::RvsdgModule & module,
        util::StatisticsCollector & statisticsCollector)
    {
        BufferInsertion pass;
        pass.Run(module, statisticsCollector);
    }
};
```

**Note on existing code**: The HLS `DeadNodeElimination.hpp/cpp` uses a free function pattern
(`EliminateDeadNodes()`) rather than extending `rvsdg::Transformation`. This should be updated
to follow the standard transformation interface in Phase 2 refactoring.

---

## 3. Implementation Tasks

### Task 2.1: Create Organized Directory Structure

- [ ] Create `passes/` subdirectories for each category
- [ ] Move existing transformation files to appropriate subdirectories
- [ ] Update all includes in `rvsdg2rhls.cpp`
- [ ] Add test fixtures for individual transformations

### Task 2.2: Reorganize Files by Category

| Original File | New Location | Tests |
|---------------|--------------|-------|
| `DeadNodeElimination.hpp/cpp` | `passes/optimization/DNE.hpp/cpp` | DNETests.cpp |
| `UnusedStateRemoval.hpp/cpp` | `passes/optimization/StateRemoval.hpp/cpp` | StateRemovalTests.cpp |
| `GammaConversion.hpp/cpp` | `passes/conversion/GammaConversion.hpp/cpp` | GammaConversionTests.cpp |
| `ThetaConversion.hpp/cpp` | `passes/conversion/ThetaConversion.hpp/cpp` | ThetaConversionTests.cpp |
| `AddBuffers.hpp/cpp` | `passes/buffering/AddBuffersPass.hpp/cpp` | AddBufferTests.cpp |
| `AddForks.hpp/cpp` | `passes/buffering/AddForksPass.hpp/cpp` | ForkInsertionTests.cpp |

### Task 2.3: Update rvsdg2rhls.cpp

Use existing `rvsdg::TransformationSequence` for transformation pipeline:

```cpp
// Existing pattern (already working):
auto gammaNodeConversion = std::make_shared<GammaNodeConversion>();
std::vector<std::shared_ptr<rvsdg::Transformation>> sequence({gammaNodeConversion, ...});
return std::make_unique<rvsdg::TransformationSequence>(sequence, dotWriter, dumpRvsdgGraphs);
```

No code changes needed - the existing `createTransformationSequence()` already works correctly.

### Task 2.4: Update Makefile.sub

Register moved test files in appropriate Makefiles.

---

## 4. Test Strategy for Phase 2

### Unit Tests for Each Transformation

```cpp
// passes/conversion/GammaConversionTests.cpp
TEST(GammaNodeConversion, ConvertsGammaToMux) {
    // Create test module with gamma node
    auto module = CreateTestModuleWithGamma();
    
    // Run transformation
    GammaNodeConversion pass;
    util::StatisticsCollector stats;
    pass.Run(module, stats);
    
    // Verify gamma replaced with mux
    EXPECT_FALSE(ContainsGammaNode(module));
    EXPECT_TRUE(ContainsMuxNode(module));
}
```

### Integration Tests

```cpp
// passes/OptimizationIntegrationTests.cpp
TEST(Transformations, CanRunInSequence) {
    auto module = CreateTestModule();
    
    DeadNodeElimination dne;
    util::StatisticsCollector stats;
    
    dne.Run(module, stats);
    // Should not crash and should reduce node count
}
```

### Regression Tests

```bash
# Run all rvsdg2rhls tests
./build/run-libhls-tests --gtest_filter=RVSDG2RHLS.*

# Run specific transformation test
./build/run-libhls-tests --gtest_filter=*GammaNodeConversion*
```

---

## 5. Files to Create/Modify

### New Directory Structure to Create
| Directory | Purpose |
|-----------|---------|
| `passes/optimization/` | Optimizations (DNE, CNE-related) |
| `passes/conversion/` | RVSDG → R-HLS conversions |
| `passes/buffering/` | Buffer/Fork/Sink insertion |
| `passes/verification/` | R-HLS verification passes |

### Existing Files to Reorganize
| Original File | New Location |
|---------------|--------------|
| `DeadNodeElimination.hpp/cpp` | `passes/optimization/DNE.hpp/cpp` |
| `UnusedStateRemoval.hpp/cpp` | `passes/optimization/StateRemoval.hpp/cpp` |
| `GammaConversion.hpp/cpp` | `passes/conversion/GammaConversion.hpp/cpp` |
| `ThetaConversion.hpp/cpp` | `passes/conversion/ThetaConversion.hpp/cpp` |
| `add-buffers.hpp/cpp` | `passes/buffering/AddBuffersPass.hpp/cpp` |

---

## 6. Success Criteria

### Phase 2 Completion Checklist
- [ ] Directory structure created and organized
- [ ] All transformation files moved to new locations
- [ ] All includes updated in rvsdg2rhls.cpp
- [ ] Makefile.sub updated with new paths
- [ ] All existing tests still pass
- [ ] New unit tests for each transformation

### Quality Metrics
| Metric | Target |
|--------|--------|
| Lines per transformation file | < 300 lines |
| Test coverage per transformation | > 80% |
| Uses rvsdg::Transformation interface | Yes |

---

## 7. Pull Request Strategy

### PR Breakdown by Transformation

Each transformation should be moved as a separate, reviewable PR:

| Transformation | Estimated Lines | PR Description |
|----------------|-----------------|----------------|
| DNE | ~100 | Move to passes/optimization/ |
| StateRemoval | ~80 | Move to passes/optimization/ |
| GammaConversion | ~150 | Move to passes/conversion/ |
| ThetaConversion | ~120 | Move to passes/conversion/ |
| MemoryStateSplit | ~200 | Move to passes/conversion/ |
| AddBuffersPass | ~80 | Move to passes/buffering/ |
| AddForksPass | ~70 | Move to passes/buffering/ |

**Guidelines from 07_PR_GUIDELINES.md:**
- Target PR size: <500 lines changed
- Single focused change per PR
- Include tests for each transformation

### Example PR Title
```
hls/rvsdg2rhls: Move GammaConversion to passes/conversion/

- Move GammaConversion.hpp/cpp to new directory structure
- Update includes in rvsdg2rhls.cpp
- Add unit test for gamma-to-mux conversion
```

---

## 8. Next Steps After Phase 2

1. Begin rhls2firrtl refactoring (Phase 3)
2. Update documentation for new directory structure
3. Add more comprehensive integration tests

---

*This phase reorganizes existing transformations into a clean directory structure while preserving the jlm convention of extending rvsdg::Transformation.*
