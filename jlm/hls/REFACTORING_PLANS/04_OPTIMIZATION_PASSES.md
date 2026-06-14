# Phase 4: HLS Optimization Passes Refactoring Plan

## Document Information
- **Phase**: 4 (Optimizations)
- **Priority**: MEDIUM - Critical optimizations
- **Status**: Planning Phase

---

## 1. Current Architecture Analysis

### Optimization Components

```
jlm/hls/opt/
├── cne.hpp/cpp                  # Common Node Elimination
├── IOBarrierRemoval.hpp/cpp     # IO barrier removal pass
└── IOStateElimination.hpp/cpp   # IO state elimination pass
```

### Key Issues Identified

1. **CNE FIXME comment** - needs clarification on relationship to LLVM version
2. **IO optimizations not well tested** - need more comprehensive tests
3. **Optimizations are transformation-based** - could benefit from unified pipeline

---

## 2. Refactoring Goals

### Phase 4.1: Common Node Elimination (CNE)

#### Current State Analysis

The HLS `CommonNodeElimination` class (`jlm/hls/opt/cne.hpp`) extends `rvsdg::Transformation` and works with HLS-specific operations like LoopOperation.

**FIXME Comment Status**: The FIXME in cne.hpp suggests generalization for use by both LLVM and HLS backends. After analysis:
- **LLVM CNE** (`jlm/llvm/opt/CommonNodeElimination.hpp`): Extends `rvsdg::Transformation`, handles complex constructs (phi nodes, loop structures)
- **HLS CNE**: Extends `rvsdg::Transformation`, adds support for HLS operations like `loop_op`

Both follow the same `Run(RvsdgModule&, StatisticsCollector&)` interface pattern.

**Recommendation**: Keep the FIXME as a reference for future work. The CNE implementations can remain separate because:
- HLS requires handling of loop-carried dependencies and buffer operations
- LLVM handles complex control flow and phi nodes differently

#### Refactoring Goals

1. **Do NOT remove the FIXME comment** - keep it as documentation of potential future generalization
2. Document that both backends use the same `rvsdg::Transformation` interface
3. Add comprehensive tests for CNE with LoopOperation
4. Consider extracting common logic if/when both backends need identical behavior

#### Key Design Pattern (CNE follows rvsdg::Transformation)

```cpp
// Both LLVM and HLS CNE follow this pattern:
class CommonNodeElimination final : public rvsdg::Transformation {
public:
    ~CommonNodeElimination() noexcept override;
    
    void Run(rvsdg::RvsdgModule & module, util::StatisticsCollector & statisticsCollector) override;

    // Optional: CreateAndRun helper for one-shot usage
    static void CreateAndRun(
        rvsdg::RvsdgModule & module,
        util::StatisticsCollector & statisticsCollector)
    {
        CommonNodeElimination pass;
        pass.Run(module, statisticsCollector);
    }
};
```

### Phase 4.2: IO Optimization Passes

#### Current State Analysis

| File | Purpose | Test Coverage |
|------|---------|---------------|
| IOBarrierRemoval | Remove IO barriers | Low |
| IOStateElimination | Eliminate unused IO state | Medium |

#### Refactoring Goals

1. Add comprehensive unit tests for both passes
2. Extract common patterns between passes (optional)
3. Document optimization semantics

---

## 3. Detailed Design

### 3.1 CNE Implementation (HLS-Specific)

```cpp
// hls/opt/cne.hpp
class CommonNodeElimination final : public rvsdg::Transformation {
public:
    ~CommonNodeElimination() noexcept override;

    void Run(rvsdg::RvsdgModule& module, util::StatisticsCollector& stats) override;
};
```

**Key Points**:
- HLS CNE focuses on eliminating duplicate nodes within HLS operations (like LoopOperation)
- Does not need to be shared with LLVM - they have different requirements
- Should test LoopOperation handling specifically

### 3.2 IO Barrier Removal

```cpp
// hls/opt/IOBarrierRemoval.hpp
class IOBarrierRemoval final : public rvsdg::Transformation {
public:
    void Run(rvsdg::RvsdgModule& module, util::StatisticsCollector& stats) override;

private:
    // Find and remove IO barrier nodes
    size_t RemoveIOBarriers(rvsdg::Region& region);

    // Check if node is an IO barrier
    bool IsIOBarrier(const rvsdg::Node* node);
};
```

### 3.3 IO State Elimination

```cpp
// hls/opt/IOStateElimination.hpp
class IOStateElimination final : public rvsdg::Transformation {
public:
    void Run(rvsdg::RvsdgModule& module, util::StatisticsCollector& stats) override;

private:
    // Find unused IO state outputs
    std::vector<rvsdg::StructuralOutput*> FindUnusedIOStates(rvsdg::Region& region);

    // Eliminate unused state edges
    void RemoveUnusedState(
        rvsdg::Region& region,
        const std::vector<rvsdg::StructuralOutput*>& toRemove);
};
```

---

## 4. Implementation Tasks

### Task 4.1: Document CNE Implementation

- [ ] **Keep the FIXME comment in `hls/opt/cne.hpp`** as future work reference
- [ ] Add documentation explaining why HLS and LLVM share the interface but not implementation
- [ ] Add test cases for LoopOperation handling

**Test Coverage**: Loop operations, regular nodes, mixed regions

### Task 4.2: IO Barrier Removal Tests

```cpp
// hls/opt/IOBarrierRemovalTests.cpp
TEST(IOBarrierRemoval, RemovesSimpleBarriers) {
    auto module = CreateModuleWithIOBarrier();

    IOBarrierRemoval pass;
    util::StatisticsCollector stats;
    pass.Run(module, stats);

    EXPECT_FALSE(ContainsIOBarrier(module));
}

TEST(IOBarrierRemoval, PreservesNonIOBarriers) {
    auto module = CreateModuleWithMixedNodes();

    IOBarrierRemoval pass;
    util::StatisticsCollector stats;
    pass.Run(module, stats);

    EXPECT_TRUE(ContainsNonIOBarrierNodes(module));
}
```

### Task 4.3: IO State Elimination Tests

```cpp
// hls/opt/IOStateEliminationTests.cpp
TEST(IOStateElimination, RemovesUnusedIOState) {
    auto module = CreateModuleWithIOState();

    IOStateElimination pass;
    util::StatisticsCollector stats;
    pass.Run(module, stats);

    EXPECT_EQ(GetIOStateCount(module), 0);
}

TEST(IOStateElimination, PreservesUsedIOState) {
    auto module = CreateModuleWithUsedIOState();

    IOStateElimination pass;
    util::StatisticsCollector stats;
    pass.Run(module, stats);

    EXPECT_GT(GetIOStateCount(module), 0);
}
```

### Task 4.4: Integration Tests

```cpp
// hls/opt/OptimizationIntegrationTests.cpp
TEST(Optimizations, CanRunInSequence) {
    auto module = CreateTestModule();

    IOBarrierRemoval barrierPass;
    IOStateElimination statePass;

    util::StatisticsCollector stats;

    barrierPass.Run(module, stats);
    statePass.Run(module, stats);

    // Should not crash and should reduce node count
}
```

### Task 4.5: Update Makefile.sub

Register new test files:
```makefile
run-libhls-tests_SOURCES += \
    jlm/hls/opt/IOBarrierRemovalTests.cpp \
    jlm/hls/opt/IOStateEliminationTests.cpp \
    jlm/hls/opt/OptimizationIntegrationTests.cpp
```

---

## 5. Test Strategy for Phase 4

### Unit Tests Checklist

| Pass | Test Cases |
|------|------------|
| CommonNodeElimination | Loop operations, regular nodes, empty regions |
| IOBarrierRemoval | Barriers in various contexts |
| IOStateElimination | Used/unused state detection |

### Regression Tests

```bash
# Run all optimization tests
./build/run-libhls-tests --gtest_filter=*Optimization*

# Run CNE tests specifically
./build/run-libhls-tests --gtest_filter=*CommonNodeElimination*

# Verify optimizations don't break other passes
./build/run-libhls-tests --gtest_filter=FullPipelineTest.*
```

---

## 6. Files to Create/Modify

### New Files to Create
| File | Purpose |
|------|---------|
| `hls/opt/IOBarrierRemovalTests.cpp` | IO barrier removal tests |
| `hls/opt/IOStateEliminationTests.cpp` | IO state elimination tests |
| `hls/opt/OptimizationIntegrationTests.cpp` | Integration tests |

### Existing Files to Modify
| File | Change |
|------|--------|
| `hls/opt/cne.hpp` | Keep FIXME comment (as future work reference), add implementation comments |
| `Makefile.sub` | Register new test files |

---

## 7. Success Criteria

### Phase 4 Completion Checklist
- [ ] CNE FIXME comment kept (as future work reference) with implementation documentation
- [ ] IOBarrierRemoval tests cover all cases
- [ ] IOStateElimination tests cover all cases
- [ ] All existing tests still pass after refactoring

### Quality Metrics
| Metric | Target |
|--------|--------|
| Test coverage per optimization | > 80% |

---

## 8. Next Steps After Phase 4

1. Update documentation for optimizations
2. Add performance benchmarks for CNE
3. Consider adding more HLS-specific optimizations

---

*This phase focuses on testing and documentation for HLS optimizations. The FIXME comment about CNE generalization is accepted as future work since HLS has its own CNE implementation for HLS-specific operations like LoopOperation.*
