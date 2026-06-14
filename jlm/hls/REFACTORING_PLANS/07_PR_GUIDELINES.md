# Pull Request Guidelines for HLS Backend Refactoring

## Document Information
- **Phase**: 7 (PR Guidelines)
- **Priority**: MEDIUM - Quality assurance
- **Status**: Updated Plan
- **Last Updated**: June 14, 2026

---

## PR Independence and Reviewability Principles

### Why Independence Matters
Each PR should be:
1. **Buildable** - Compiles without unmerged dependencies
2. **Testable** - Tests pass in isolation from other PRs
3. **Reversible** - Can be reverted independently if needed
4. **Reviewable** - Small enough to review thoroughly

### Independence Checklist (Per PR)
- [ ] All dependency PRs are merged (check graph below)
- [ ] No public API breaking changes unless documented
- [ ] Tests pass with existing codebase
- [ ] Documentation updated for any user-facing changes

---

## 1. Overview: Why PR Size Matters

### The Reviewability Problem
From the refactoring plans, we identified several large files that need splitting:

| File | Lines | Risk if One PR |
|------|-------|----------------|
| `RhlsToFirrtlConverter.cpp` | 4126 | Critical - too complex to review |
| `hls.hpp` | 1760 | High - too many changes at once |
| `rvsdg2rhls.cpp` | 479 | Medium - still large |

### Best Practice: Small, Focused PRs

**Target Size Guidelines:**
- **Ideal PR**: 50-200 lines changed
- **Maximum PR**: 300-500 lines changed (rare exceptions)
- **PR Review Time**: <15 minutes for ideal, <30 minutes for max

### Exceptions to Maximum Size
| Scenario | Max Lines | Example |
|----------|-----------|---------|
| Generator interface + registry | 400 | Base infrastructure with tests |
| Memory generator (split needed) | 400 | Separate Load/Store PRs |
| IR module with full tests | 350 | Class + comprehensive tests |

**From Phase 3 Success Criteria:**
> Lines per generator file: **<300 lines**

#### Critical Split Rules
1. If a single operation group exceeds ~400 lines, split it
2. Generator PRs must include test coverage per operation
3. Orchestration updates should come after all generators

---

## 2. PR Structure Template

### PR Title Format
```
hls/<component>: <one-line-summary>

[Refactoring Type]
- [rvsdg2rhls]: Transformation reorganization
- [rhls2firrtl]: Generator extraction
- [ir]: IR layer modularization
- [opt]: Optimization pass improvements

[PR Type]
- feat: New feature (add a generator)
- refactor: Code restructuring only
- test: Test additions/changes
- docs: Documentation updates
```

**Examples:**
```
hls/rhls2firrtl: Extract arithmetic operations to generator

- Create ArithmeticOpGenerator class
- Move Add/Sub/Xor/Mul logic from RhlsToFirrtlConverter.cpp
- Add unit tests for each operation
- Update generator registry

Refactoring Type: Generator pattern extraction
PR Type: refactor + test
```

```
hls/rvsdg2rhls: Create organized directory structure for passes

- Move transformations to passes/ subdirectories
- Group by category (optimization, conversion, buffering)
- No code changes - only file reorganization

Refactoring Type: Directory organization
PR Type: refactor + test
```

---

## 3. PR Dependency Graph and Order

### Refactoring Phase Dependencies
```
Phase 0 (Infrastructure) ──┐
├── GeneratorInterface     │
└── GeneratorRegistry      ├──→ Phase 2 (rvsdg2rhls)
                                ├── passes/organize/ directory structure
                                └── rvsdg2rhls.cpp update (final)

                            Phase 3 (rhls2firrtl)
                            ├── generators/ interface + registry
                            │   └── [Each Generator PR depends on Registry]
                            └── RhlsToFirrtlConverter orchestrator (final)

                            Phase 5 (IR Layer)
                            ├─┬ types/ BundleType, TriggerType
                            │ └── operations/ (ordered by dependencies)
                            │       ├── BranchOperation (base)
                            │       ├── ForkOperation (depends on Branch)
                            │       ├── MuxOperation (depends on Branch)
                            │       ├── BufferOperation
                            │       └── ControlOperations
                            └── nodes/ LoopNode
```

### PR Execution Order by Phase
| Phase | PR Order | Notes |
|-------|----------|-------|
| 0 | Interface → Registry | Infrastructure first |
| 2 | Reorganize all → Update orchestrator | Final step |
| 3 | Interface → All generators → Orchestrator | Orchestrator last |
| 5 | types/ → operations/ (dependency order) → nodes/ | Follow dependency graph |

---

## 4. PR Checklist Template

### Pre-Submission Checklist
```markdown
# PR Checklist

## Build & Test
- [ ] All existing tests pass (`make check`)
- [ ] No new compiler warnings
- [ ] New tests added for refactored code
- [ ] Test coverage maintained or improved

## Code Quality
- [ ] `clang-format` applied (from project root `.clang-format`)
- [ ] `clang-tidy` passes (from project root `.clang-tidy`)
- [ ] Follows jlm coding conventions:
  - [ ] PascalCase for classes
  - [ ] camelCase for variables
  - [ ] Uses `jlm::hls` namespace

## Architecture Compliance
- [ ] Follows rvsdg::Transformation interface (if applicable)
- [ ] Uses JLM_UNREACHABLE instead of exceptions
- [ ] Single responsibility per file/class
- [ ] No functional changes (refactoring only)

## Documentation
- [ ] Doxygen comments for public classes
- [ ] PR description includes summary
- [ ] References relevant refactoring plan document

## Risk Assessment
- [ ] PR size <500 lines changed
- [ ] No API breaking changes
- [ ] Rollback point established (git branch/tag)

## Independence Checklist (NEW)
- [ ] All dependency PRs are merged (see dependency graph above)
- [ ] Can build without other unmerged PRs
- [ ] Tests pass in isolation from other PRs
```

---

## 5. Phase-Specific PR Strategy

### Phase 1: Test Infrastructure
**Goal**: Establish testing before any refactoring

| PR | Description | Size Target |
|----|-------------|-------------|
| 1 | Create test fixtures base class | <100 lines |
| 2 | Add FIRRTL verification helpers | <150 lines |
| 3 | Add pattern reuse infrastructure | <200 lines |

**Key**: Tests must pass for ALL existing functionality BEFORE starting refactoring

---

### Phase 2: rvsdg2rhls Refactoring
**Goal**: Organize transformations into passes/

#### Updated PR Breakdown (11 PRs)
```
passes/
├── optimization/
│   ├── DNE.hpp/cpp         → PR #1 (transformations/optimization/)
│   └── StateRemoval.hpp/cpp → PR #2 (transformations/optimization/)
├── conversion/
│   ├── GammaConversion     → PR #3
│   ├── ThetaConversion     → PR #4
│   ├── MemoryStateSplit    → PR #5
│   └── StreamConversion    → PR #6
├── buffering/
│   ├── AddBuffersPass      → PR #7
│   ├── AddForksPass        → PR #8
│   └── AddSinksPass        → PR #9
└── verification/
    └── CheckRhls           → PR #10

orchestrator/ (FINAL STEP)
└── rvsdg2rhls.cpp update  → PR #11 (includes for new passes/)
```

**Each PR Should Include:**
- [ ] File moved to new location
- [ ] Update includes in `rvsdg2rhls.cpp` (PR #11 only)
- [ ] Update Makefile.sub with test file path
- [ ] Test added for individual transformation

#### Phase 2 Independence Notes
- PRs #1-10 are independent of each other (can be merged in any order)
- PR #11 **must** come last (updates orchestrator to use new paths)
- Each PR is fully buildable and testable standalone

---

### Phase 3: rhls2firrtl Refactoring
**Goal**: Extract operation generators from RhlsToFirrtlConverter.cpp (4126 lines!)

#### Updated PR Splitting Strategy (10-12 PRs for better reviewability)
| PR | Operation Category | Lines Moved | Tests | Notes |
|----|-------------------|-------------|-------|-------|
| 1 | Generator interface + registry | ~350 lines | Integration tests | **Phase 0 required first** |
| 2 | Arithmetic ops (Add, Sub) | ~200 lines | Unit tests per op | Split large group |
| 3 | Arithmetic ops (And, Or, Xor) | ~200 lines | Unit tests per op | Balanced split |
| 4 | Arithmetic ops (Mul, Div, Shifts) | ~200 lines | Unit tests per op | Balanced split |
| 5 | Comparison ops (Eq, Neq, Lt, Gt, Leq, Geq) | ~150 lines | Unit tests | OK as-is |
| 6a | Memory ops (Mem, Load) | ~400 lines | Integration tests | Split to avoid >400 |
| 6b | Memory ops (Store, LocalMem) | ~400 lines | Integration tests | Separate PRs |
| 7a | Control flow (Fork, StateGate) | ~300 lines | Integration tests | Split control flow |
| 7b | Control flow (Buffer, Trigger) | ~300 lines | Integration tests | Separate PRs |
| 8 | Structural (Sink, Print, ExtModule) | ~200 lines | Unit tests | OK as-is |
| 9 | Refactor orchestrator to use generators | ~500 lines | Full pipeline test | **Final step** |

**Total**: 10-12 PRs for Phase 3

#### Independence Order for Phase 3
```
PR #1 (Interface) → [PRs #2-8 in any order] → PR #9 (Orchestrator)
```

#### For Each Generator PR:
- [ ] Create new generator class (canHandle, generate methods)
- [ ] Extract operation handling logic
- [ ] Add unit tests for each operation
- [ ] Register in generator registry
- [ ] Update RhlsToFirrtlConverter to use generators

#### Critical Split Rules Applied
1. Memory ops split into two ~400 line PRs (6a/6b) instead of one 800-line PR
2. Control flow split into two ~300 line PRs (7a/7b) instead of one 600-line PR
3. All arithmetic operations split across three balanced PRs

---

### Phase 4: Optimization Passes
**Goal**: Test and document existing optimizations

#### PR Breakdown (unchanged - already optimal)
| PR | Description | Notes |
|----|-------------|-------|
| 1 | Document CNE FIXME comment (keep for future work) | Documentation only |
| 2 | Add IOBarrierRemoval tests | Standalone test file |
| 3 | Add IOStateElimination tests | Standalone test file |
| 4 | Integration tests for optimization sequence | Combines both passes |

#### Independence Notes
- All PRs are independent and can be merged in any order
- Each PR is fully buildable and testable standalone

---

### Phase 5: IR Layer Refactoring
**Goal**: Split hls.hpp (~1760 lines) into modules with proper dependency ordering

#### Updated PR Breakdown by Module (ordered by dependencies)
```
ir/
├── types/                → PR #1a (BundleType), PR #1b (TriggerType)
│   ├── BundleType.hpp/cpp
│   └── TriggerType.hpp/cpp
├── operations/           → PRs #2-6 (dependency order)
│   ├── BranchOperation   → PR #2 (base, no dependencies)
│   ├── ForkOperation     → PR #3 (may depend on Branch)
│   ├── MuxOperation      → PR #4 (may depend on Branch)
│   ├── BufferOperation   → PR #5
│   └── ControlOperations → PR #6 (grouped, depends on others)
└── nodes/                → PR #7 (LoopNode, others)
    └── LoopNode.hpp/cpp
```

#### Dependency-Aware Order
| PR | Module | Notes |
|----|--------|-------|
| 1a | BundleType | No dependencies |
| 1b | TriggerType | No dependencies |
| 2 | BranchOperation | Base operation, no deps |
| 3 | ForkOperation | May use Branch types |
| 4 | MuxOperation | May use Branch types |
| 5 | BufferOperation | May use other ops |
| 6 | ControlOperations | Grouped operations |
| 7 | LoopNode | May depend on ops |

#### Each Module PR Should Include:
- [ ] Class moved to new location
- [ ] Doxygen documentation added
- [ ] Unit tests for operation methods (equality, copy, hash)
- [ ] Update main hls.hpp header to include modules (final PR only)

#### Independence Notes
- Types PRs (#1a, #1b) are independent
- Operations ordered by likely dependencies
- Main `hls.hpp` update is final step

---

### Phase 6: Test Infrastructure
**Goal**: Comprehensive test coverage for all HLS components

#### PR Breakdown (unchanged - already optimal)
| PR | Description | Notes |
|----|-------------|-------|
| 1 | Add test fixtures base class | Foundation for others |
| 2 | Add FIRRTL verification helpers | Can be used by tests |
| 3 | Add pattern reuse (ControlFlowPatterns, MemoryAccessPatterns) | Reusable patterns |
| 4 | Integration tests for full pipeline | Combines infrastructure |

#### Independence Notes
- All PRs are independent and can be merged in any order
- Each PR is fully buildable and testable standalone

---

## 6. Refactoring-Only Commit Messages

### Template
```
hls/<component>: <action> for refactoring

<specific-action>

[From Phase N of REFACTORING_PLANS/00_OVERVIEW.md]
```

### Examples
```bash
# Phase 2 - Directory reorganization
git commit -m "hls/rvsdg2rhls: Create organized passes/ directory structure

- Move transformations to passes/ subdirectories by category
- Group optimization, conversion, buffering, verification separately
- No code changes - only file reorganization

[From Phase 2 of REFACTORING_PLANS/00_OVERVIEW.md]"

# Phase 3 - Generator extraction
git commit -m "hls/rhls2firrtl: Create ArithmeticOpGenerator for add/sub ops

- Implement generator interface with canHandle() and generate()
- Extract Add/Sub/Xor logic from RhlsToFirrtlConverter.cpp (~400 lines)
- Add unit tests for each operation (32-bit, 64-bit variants)

[From Phase 3 of REFACTORING_PLANS/00_OVERVIEW.md]"

# Phase 5 - Module extraction
git commit -m "hls/ir: Extract BundleType to types module

- Move BundleType class to ir/types/
- Add Doxygen documentation for bundle type semantics
- Update main hls.hpp to include module header

[From Phase 5 of REFACTORING_PLANS/00_OVERVIEW.md]"
```

---

## 7. PR Review Best Practices

### For Authors
1. **Title should be descriptive**: "hls/rhls2firrtl: Extract arithmetic operations" not "Refactor"

2. **First line is summary**: Keep first line under 70 characters, describe what changed

3. **Body explains why**: Reference refactoring plan documents:
   ```
   Refactoring as described in jlm/hls/REFACTORING_PLANS/03_RHLS2FIRRTL_REFACTORING.md
   ```

4. **Link to related PRs**: If multiple PRs are related, reference them

5. **Check independence**: Before submitting, verify all dependency PRs are merged

### For Reviewers
1. **Check build first**: Does it compile without warnings?
   ```bash
   make clean && make -j$(nproc)
   ```

2. **Run tests**: 
   ```bash
   ./build/run-libhls-tests --gtest_brief=0
   ```

3. **Verify no functional changes**: 
   - Test output should be identical (if refactoring only)
   - Check git diff for unintended code modifications

4. **Check PR size**: If >500 lines, consider requesting split into smaller PRs

5. **Check independence**: Verify dependency PRs are already merged

---

## 8. Rollback Strategy Per PR

### Before Merging Each PR:
```bash
# Create rollback point
git checkout -b pre-pr-<number>-backup
git add .
git commit -m "Pre-PR #<number> backup (date)"
```

### If Issues Detected After Merge:
```bash
# Quick revert (if clean history)
git revert <commit-hash>

# Full rollback (if needed)
git reset --hard pre-pr-<number>-backup
```

---

## 9. PR Summary Template

### Use This for Each Refactoring Phase

```markdown
# Phase N Refactoring - Pull Request Summary

## Overview
[Brief description of phase goal]

## PRs Created
| # | Description | Lines Changed | Status |
|---|-------------|---------------|--------|
| 1 | [PR title] | ~XXX | [ ] Open / [x] Merged |

## Testing Results
- [ ] All existing tests pass
- [ ] New tests added for refactored code
- [ ] Code coverage maintained

## Verification Checklist
- [ ] Build successful (no warnings)
- [ ] clang-format applied
- [ ] clang-tidy passes
- [ ] No functional changes introduced

## Next Steps
[What remains to complete the phase]
```

---

## 10. Quick Reference: File Size Guidelines

### Splitting Thresholds by File Type
| Original File | Lines | Recommended PRs | Notes |
|--------------|-------|-----------------|-------|
| RhlsToFirrtlConverter.cpp | 4126 | ~9-10 PRs (generators) + 1 (orchestrator) | Split large generators |
| hls.hpp | 1760 | ~8 PRs (modules, ordered by deps) | Add dependency ordering |
| rvsdg2rhls.cpp | 479 | ~1-2 PRs (organize passes) | Include orchestrator update |

### Rule of Thumb
Split if file >300 lines OR has >5 distinct components

#### Splitting Guidance for Large Files
| Scenario | Action |
|----------|--------|
| Single operation group >400 lines | Split into 2 PRs (~200-300 each) |
| Multiple independent transformations | One PR per transformation |
| Operations with dependencies | Order by dependencies, not size |

---

## 11. Success Metrics for Refactoring PRs

### Definition of Done for Each PR:
```markdown
- [ ] All existing tests pass
- [ ] New tests added (if applicable)
- [ ] Build clean (no warnings)
- [ ] Code follows conventions (clang-format, clang-tidy)
- [ ] Documentation updated if needed
- [ ] PR size <500 lines
- [ ] Single focused change
- [ ] No breaking changes
```

---

## 12. PR Count Summary

| Phase | Original Estimate | Recommended (with independence focus) |
|-------|-------------------|--------------------------------------|
| Phase 0 (Infrastructure) | 2 PRs | 2 PRs ✅ |
| Phase 1 (Test Setup) | 3 PRs | 3 PRs ✅ |
| Phase 2 (rvsdg2rhls) | ~10 PRs | **11 PRs** (add orchestrator update) |
| Phase 3 (rhls2firrtl) | 8 PRs | **10-12 PRs** (split large generators) |
| Phase 4 (Optimizations) | 4 PRs | 4 PRs ✅ |
| Phase 5 (IR Layer) | 7 PRs | **9 PRs** (add dependency order) |
| Phase 6 (Test Infra) | 4 PRs | 4 PRs ✅ |
| **Total** | ~40 PRs | **~38-41 PRs** |

### Why More PRs Improve Reviewability
1. **Smaller reviews**: <200 lines per PR is ideal for thorough review
2. **Easier bisecting**: Smaller changes are easier to identify issues with git bisect
3. **Faster merges**: Can merge independent PRs while others are being reviewed
4. **Lower risk**: If one PR has issues, fewer changes need rollback

---

*This document provides PR guidelines for the HLS backend refactoring. Each PR should reference this guide and follow the established patterns.*

---

### Updated: June 14, 2026 - Added PR Independence Principles

## Unit Test PR Strategy for `feature/hls-tests` Branch

**Goal:** Integrate the HLS unit tests that reside in the `feature/hls-tests` branch into master via dedicated pull requests.

### Why Separate PRs?
- **Isolation:** Adds test files without altering existing refactoring code, preserving buildability.
- **Reviewability:** Small test‑only PRs (< 200 lines) are easy to review and approve.
- **CI Assurance:** Each PR can be CI‑tested independently, ensuring master never regresses.

### Suggested Workflow
1. **Create a base branch** from `master` named `hls-tests-integration`.
2. **Sync the test branch:**
   ```bash
   git fetch origin feature/hls-tests
   git checkout -b hls-tests-sync origin/feature/hls-tests
   ```
3. **Group tests logically** (e.g., generator‑specific, pass‑specific, end‑to‑end).  
   Each group becomes its own PR.
4. **Open a PR per group** targeting `master`:
   - Title format:  
     `hls/tests: Add <group description> unit tests`
   - Description should include:
     * List of added test files.
     * Any new test utilities or fixtures required.
     * Reference to the relevant refactoring phase (e.g., Phase 3 generator PRs).

### Checklist per Test‑Addition PR
- [ ] All new test files compile (`make check` passes).
- [ ] No existing tests are broken.
- [ ] New tests achieve ≥ 80 % line coverage for exercised code.
- [ ] Updated `CMakeLists.txt` or Makefile entries added.
- [ ] Documentation in `docs/Testing.md` updated if new fixtures/utilities introduced.
- [ ] CI pipeline passes on the PR.

### Dependency Notes
- These test PRs **must be merged after** their corresponding refactoring PRs that introduce the code they exercise (e.g., generator PRs before generator tests).
- They can be merged in any order relative to each other, provided the underlying implementation exists.

### Post‑Merge Actions
- Run the full HLS test suite (`./build/run-libhls-tests`) on master.
- Update the **Test Infrastructure** section of the overall refactoring plan if new fixtures were added.


**Key Changes:**
- Added "PR Independence and Reviewability Principles" section
- Added dependency graph showing PR order
- Split Phase 3 generators into more reviewable chunks (10-12 vs 8 PRs)
- Split Phase 5 operations by dependency order (9 vs 7 PRs)
- Added independence checklist to PR template
- Added PR count summary with rationale