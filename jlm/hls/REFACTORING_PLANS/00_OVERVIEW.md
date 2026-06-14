# HLS Backend Refactoring Plan - Overview

## Document Information
- **Version**: 2.0
- **Date**: June 14, 2026 (Updated with analysis feedback)
- **Status**: Updated Plan
- **Target**: JLM Compiler Project

### jlm Conventions Used in This Refactoring

This refactoring plan follows jlm's coding conventions:

#### 1. Transformation Interface
Transformations extend `rvsdg::Transformation` with `Run(RvsdgModule&, StatisticsCollector&)`:
```cpp
class GammaNodeConversion final : public rvsdg::Transformation {
public:
  ~GammaNodeConversion() noexcept override;
  void Run(rvsdg::RvsdgModule & module, util::StatisticsCollector & statisticsCollector) override;
};
```
See `jlm/hls/backend/rvsdg2rhls/GammaConversion.hpp` for examples.

#### 2. Service/Summarizer Classes
Some components follow a different pattern where they extend their own base class:
```cpp
class AgnosticModRefSummarizer final : public ModRefSummarizer {
public:
  std::unique_ptr<ModRefSummary> SummarizeModRefs(
      const rvsdg::RvsdgModule & module,
      const PointsToGraph & pointsToGraph,
      util::StatisticsCollector & statisticsCollector);
};
```
See `jlm/llvm/opt/alias-analyses/AgnosticModRefSummarizer.hpp` for examples.

#### 3. Test Fixtures
Use `llvm::LlvmRvsdgModule` as the primary module type for HLS tests.

#### 4. Error Handling
Use `JLM_UNREACHABLE("message")` instead of exceptions for unrecoverable errors.
Note: Some existing code may use `throw std::logic_error()`; prefer `JLM_UNREACHABLE` in new code.

#### 5. Namespace Organization
- Use `namespace jlm::hls` for HLS‑specific code
- Use `namespace jlm::llvm::aa` for LLVM alias‑analysis code (separate concern)

See individual phases for more details.

---

## 1. Introduction

This document provides a comprehensive refactoring plan for the HLS backend of the JLM compiler. The goal is to improve code organization, maintainability, and testability while preserving all existing functionality.

### Current State Analysis

#### Code Organization Issues
| Component | File Count | Lines (approx) | Major Issues |
|-----------|------------|----------------|--------------|
| rvsdg2rhls | 38 | ~4000 | Monolithic, mixed concerns |
| rhls2firrtl | 9 | ~5000+ | RhlsToFirrtlConverter.cpp too large (4126 lines) |
| opt | 6 | ~1000 | CNE optimization duplicated logic |

#### Key Findings
1. **RhlsToFirrtlConverter.cpp** is 4126 lines - needs to be split by operation type  
2. **rvsdg2rhls transformations** are not well‑organized as a pipeline  
3. **BaseHLS** provides common functionality but can be improved  
4. **Test coverage** exists but test organization could be better  

---

## 2. Refactoring Principles

### Core Guiding Principles
1. **Single Responsibility**: Each file/class should have one clear purpose  
2. **Layered Architecture**: Clear separation between transformation and generation layers  
3. **Testability First**: Every refactored component must be testable in isolation  
4. **Progressive Enhancement**: Changes should be incremental with tests at each step  

### Design Patterns to Apply
| Pattern | Application |
|---------|-------------|
| Strategy Pattern | Operation‑specific generators |
| Factory Pattern | Node handler creation |
| Builder Pattern | FIRRTL generation |
| Visitor Pattern | AST traversal (alternative to large switch) |

---

## 3. Directory Structure

```
jlm/hls/
├── backend/
│   ├── rvsdg2rhls/               # RVSDG → R‑HLS transformations
│   │   ├── passes/                # NEW: Organized transformation classes
│   │   │   ├── optimization/      # DNE, CNE‑related
│   │   │   ├── conversion/        # Gamma, Theta, Memory conversions
│   │   │   ├── buffering/         # AddBuffers, AddForks, etc.
│   │   │   └── verification/      # R‑HLS verification passes
│   │   └── rvsdg2rhls.hpp/cpp     # Main orchestrator (uses existing TransformationSequence)
│   │
│   ├── rhls2firrtl/               # R‑HLS → FIRRTL generation
│   │   ├── generators/            # NEW: Operation‑specific generators
│   │   │   ├── GeneratorInterface.hpp/cpp
│   │   │   ├── GeneratorRegistry.hpp/cpp
│   │   │   ├── ArithmeticOpGenerator.hpp/cpp
│   │   │   ├── MemoryOpGenerator.hpp/cpp
│   │   │   └── ...
│   │   ├── builder/               # NEW: FIRRTL construction helpers
│   │   │   ├── FirrtlBuilder.hpp/cpp
│   │   │   ├── RegisterBuilder.hpp/cpp
│   │   │   └── WhenElseBuilder.hpp/cpp
│   │   └── RhlsToFirrtlConverter.hpp/cpp  # Main orchestrator (uses generators)
│   │
│   └── firrtl2verilog/            # FIRRTL → Verilog lowering
│       └── FirrtlToVerilogConverter.hpp/cpp
├── opt/                           # HLS‑specific optimizations
│   ├── cne.cpp                    # Common Node Elimination (HLS‑specific)
│   ├── IOBarrierRemoval.hpp/cpp
│   └── IOStateElimination.hpp/cpp
└── ir/                            # HLS IR types and operations
    ├── types/                     # NEW: BundleType, TriggerType
    │   ├── BundleType.hpp/cpp
    │   └── TriggerType.hpp/cpp
    ├── operations/                # NEW: BranchOperation, etc.
    │   ├── BranchOperation.hpp/cpp
    │   ├── ForkOperation.hpp/cpp
    │   └── ...
    ├── nodes/                     # NEW: LoopNode, etc.
    │   └── LoopNode.hpp/cpp
    └── hls.hpp/cpp                # Main header (includes modules)
```

---

## 4. Refactoring Phases

### Phase 0: Infrastructure Spike (CURRENT) - **NEW**
**Priority**: CRITICAL - Must be done before any refactoring begins  
This phase addresses critical issues identified in the analysis:

#### Phase 0.1: Generator Interface Implementation
- [ ] Create `generators/GeneratorInterface.hpp/cpp`
- [ ] Define interface with proper error handling (no JLM_UNREACHABLE)
- [ ] Add `GetSupportedOperations()` method for diagnostics

#### Phase 0.2: Generator Registry Infrastructure
- [ ] Implement thread‑safe registration system with mutex protection
- [ ] Design error messages for missing generators

#### Phase 0.3: Backward Compatibility Layer
- [ ] Create symlink/forwarding includes for `ir/hls.hpp` migration
- [ ] Plan directory reorganization with include path updates

**Deliverable**: Infrastructure ready for Phase 1 implementation  

---

### Phase 1: Test Infrastructure & Verification (Week 1)
**Priority**: CRITICAL - Must be completed before any code changes  
- [ ] Create `REFACTORING_PLANS/` directory structure
- [ ] Write unit tests for all existing functionality
- [ ] Set up continuous integration for refactoring verification
- [ ] Document test coverage requirements  

**Deliverable**: `01_SETUP_VERIFICATION.md`

---

### Phase 2: rvsdg2rhls Refactoring (Weeks 2‑3)
**Priority**: HIGH - Foundation layer  
- [ ] Create `passes/` directory with PassPipeline class
- [ ] Extract each transformation to its own file
- [ ] Add test fixtures for individual transformations
- [ ] Update `rvsdg2rhls.cpp` to use pipeline  

**Deliverable**: `02_RVSDG2RHLS_REFACTORING.md`

---

### Phase 3: rhls2firrtl Refactoring (Weeks 4‑6)
**Priority**: HIGH - Core conversion logic  
- [ ] Create `generators/` directory structure
- [ ] Extract arithmetic, memory, control‑flow, and structural operations into generators
- [ ] Implement FIRRTL Builder helpers
- [ ] Refactor `RhlsToFirrtlConverter` to use generators  

**Deliverable**: `03_RHLS2FIRRTL_REFACTORING.md`

---

### Phase 4: HLS Optimization Passes (Week 7)
**Priority**: MEDIUM - Critical optimizations  
- [ ] Fix CNE FIXME about generalization
- [ ] Add test coverage for all optimization passes
- [ ] Verify transformations don’t break semantics  

**Deliverable**: `04_OPTIMIZATION_PASSES.md`

---

### Phase 5: HLS IR Layer Refactoring (Week 8)
**Priority**: MEDIUM - Foundation  
- [ ] Review and clean up operation definitions
- [ ] Add proper documentation to all classes
- [ ] Consolidate related types  

**Deliverable**: `05_IR_LAYER_REFACTORING.md`

---

### Phase 6: End‑to‑End Validation (Ongoing)
**Priority**: HIGH - Quality assurance  
- [ ] Document testing best practices
- [ ] Create test fixtures for common patterns
- [ ] Add mock interfaces for FIRRTL dialect  

**Deliverable**: `08_E2E_VALIDATION.md`

---

### Phase 7: Unit Testing Guidelines (Ongoing)
**Priority**: HIGH - Quality assurance  
- [ ] Document testing best practices
- [ ] Create test fixtures for common patterns
- [ ] Add mock interfaces for FIRRTL dialect  

**Deliverable**: `06_TEST_INFRASTRUCTURE.md`

---

### Phase 8: Pre‑Refactor Benchmarking (Ongoing) - **NEW**
**Priority**: CRITICAL - Must be completed before any refactoring begins  
- [ ] Capture unit test baseline performance
- [ ] Record external test suite golden values
- [ ] Store FIRRTL output hashes for equivalence checks
- [ ] Document compilation time metrics
- [ ] Create pre‑refactor verification script  

**Deliverable**: `09_PRE_REFACTOR_BENCHMARKING.md`

---

## Updated Phase Ordering

To improve logical flow the phases are renumbered as follows:

| New Phase | Original Phase | Description |
|-----------|----------------|-------------|
| 1️⃣ | 1 (Setup Verification) | Pre‑refactor verification and test infrastructure |
| 2️⃣ | 2 (rvsdg2rhls) | Reorganize RVSDG → R‑HLS transformations |
| 3️⃣ | 3 (rhls2firrtl) | Extract operation generators |
| 4️⃣ | 4 (Optimization Passes) | Test and document optimizations |
| 5️⃣ | 5 (IR Layer) | Split `hls.hpp` into modules |
| 6️⃣ | 6 (Test Infrastructure) | Establish fixtures, patterns, integration tests |
| 7️⃣ | 9 (Pre‑Refactor Benchmarking) | Capture baseline performance & coverage before any changes |
| 8️⃣ | 8 (End‑to‑End Validation) | FIRRTL→Verilog lowering, Verilator simulation, regression checks |

All cross‑references in the individual phase documents should be updated accordingly.

---

## Additional Improvements

* **Dashboard** – Create a top‑level `REFRACTORING_OVERVIEW.md` that contains a status table with links to each detailed plan.
* **Centralised Guidelines** – Move coding conventions and error‑handling policy to `docs/DEVELOPMENT_GUIDELINES.md` and reference it from the phase documents.
* **GitHub Actions CI** – Add dedicated jobs:
  * `baseline-check`
  * `performance-check`
  * `e2e-validation`
* **Workflow File** – See `.github/workflows/hls_refactor_ci.yml`.

---

## Implementation Tasks Summary

| Task | Description |
|------|-------------|
| Create `REFRACTORING_OVERVIEW.md` dashboard |
| Centralise conventions in `docs/DEVELOPMENT_GUIDELINES.md` |
| Add GitHub Actions workflow (`.github/workflows/hls_refactor_ci.yml`) |
| Update cross‑references to new phase numbers |
| Adjust Makefile.sub entries for new test files and generators |
| Ensure all PRs obey the <400 lines limit (enforced via CI) |

---

## Success Criteria

* All phases completed according to the renumbered order.
* CI pipeline runs baseline, performance, and E2E jobs on every PR.
* No functional regressions; test coverage ≥ 90 % for HLS components.
* Documentation and dashboards are up‑to‑date.

---

*This updated overview reflects the refined phase ordering and additional process improvements required to ensure a smooth, reviewable refactoring of the HLS backend.*