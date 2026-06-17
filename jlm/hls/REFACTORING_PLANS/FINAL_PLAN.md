# HLS Backend Refactoring – Final Consolidated Plan

## 📋 Summary Table

| Phase | Document | Priority | Estimated Duration | Status | Dependencies |
|-------|----------|----------|-------------------|--------|--------------|
| 0️⃣ **Infrastructure** | [00_INFRASTRUCTURE.md](00_INFRASTRUCTURE.md) | CRITICAL | 2 days | Planned | None (can start anytime) |
| 1️⃣ **Setup & Verification** | [01_SETUP_VERIFICATION.md](01_SETUP_VERIFICATION.md) | CRITICAL | 3‑5 days | In Progress | None |
| 2️⃣ **RVSDG → RHLS Refactoring** | [02_RVSDG2RHLS_REFACTORING.md](02_RVSDG2RHLS_REFACTORING.md) | HIGH | 1‑2 weeks | Planned | Phase 1 |
| 3️⃣ **RHLS → FIRRTL Refactoring** | [03_RHLS2FIRRTL_REFACTORING.md](03_RHLS2FIRRTL_REFACTORING.md) | HIGH | 2‑3 weeks | Planned | Phase 1, 0 (optional) |
| 4️⃣ **Optimization Passes** | [04_OPTIMIZATION_PASSES.md](04_OPTIMIZATION_PASSES.md) | MEDIUM | 1 week | Planning | Phase 2 |
| 5️⃣ **IR Layer Refactoring** | [05_IR_LAYER_REFACTORING.md](05_IR_LAYER_REFACTORING.md) | MEDIUM | 1 week | Planned | None (independent) |
| 6️⃣ **Test Infrastructure** | [06_TEST_INFRASTRUCTURE.md](06_TEST_INFRASTRUCTURE.md) | CRITICAL | Ongoing | In Progress | Phase 1 |
| 7️⃣ **PR Guidelines & CI** | [07_PR_GUIDELINES.md](07_PR_GUIDELINES.md) | HIGH | Ongoing | In Progress | None |
| 8️⃣ **End‑to‑End Validation** | [08_E2E_VALIDATION.md](08_E2E_VALIDATION.md) | HIGH | Ongoing | Planned | Phase 3, 6 |
| 9️⃣ **Pre‑Refactor Benchmarking** | [09_PRE_REFACTOR_BENCHMARKING.md](09_PRE_REFACTOR_BENCHMARKING.md) | CRITICAL | 3 days | Planned | Phase 1 |

*All phases are limited to modifications **only inside `jlm/hls/`**.*

### 🔗 Dependency Graph (Updated June 14, 2026)

```
Phase 1 (Setup) ──┬──→ Phase 2 (rvsdg2rhls)
                  ├──→ Phase 3 (rhls2firrtl) [needs generator infrastructure if using generators]
                  ├──→ Phase 4 (Optimizations)
                  ├──→ Phase 5 (IR Layer) ← independent!
                  ├──→ Phase 6 (Tests)
                  └──→ Phase 9 (Benchmarking)

Phase 0 (Infrastructure) ──▶ Phase 3 (rhls2firrtl generators) [if using generator pattern]
                             Optional: can defer until needed for rhls2firrtl
```

**Key Dependency Clarifications:**
- **Phase 5 (IR Layer)** is independent of other phases - file reorganization only
- **Phase 0 (Infrastructure)** is optional - only required if using the new generator pattern in Phase 3
- **Phase 2** depends on Phase 1 for test infrastructure, not Phase 0

## 🔧 Reuse Findings – Leveraging Existing Project Code

- **Utility Helpers** – `jlm/util/common.hpp` provides `JLM_UNREACHABLE`, assertions, and safe casts.  
- **File I/O** – `jlm/util/file.hpp` offers `FilePath`, unique temporary file creation, and path utilities for template loading and benchmark artefacts.  
- **Statistics** – `jlm/util/Statistics.{hpp,cpp}` supplies a ready‑to‑use `StatisticsCollector`; all new passes/generators should record per‑operation metrics.  
- **RVSDG Core** – The transformation interface (`rvsdg::Transformation`), node hierarchy, and `TransformationSequence` are fully reusable for any new HLS pass.  
- **LLVM Converter Logic** – `jlm/llvm/backend/IpGraphToLlvmConverter.hpp/.cpp` contains a comprehensive set of `convert_*` methods that map RVSDG operations to lower‑level IR. These can be copied verbatim and adapted to the FIRRTL builder API, eliminating the need to write new dispatch code for arithmetic, memory, and control‑flow ops.  
- **Test Helpers** – Functions in `jlm/llvm/TestRvsdgs.hpp` that construct small RVSDG snippets (add, mul, gamma, memory) should be used in all generator unit tests.  
- **DOT Writer** – `jlm/rvsdg/DotWriter` remains the standard way to visualise intermediate graphs for debugging failed passes.

All of the above live outside `jlm/hls`; they can be linked and included without any modification, satisfying the constraint that only `jlm/hls/` may be changed.

## 🔗 Navigation
- ⬆️ Back to [00_OVERVIEW.md](00_OVERVIEW.md)
- ⬇️ Forward to detailed phase documents (see table above)

### Additional Note
- The **Unit Test PR Strategy** for integrating HLS unit tests from the `feature/hls-tests` branch has been added to the PR Guidelines document (`07_PR_GUIDELINES.md`). This outlines how to create focused pull requests for those tests, ensuring they are reviewed and merged after the corresponding refactoring work.

---

## 🛡️ Risk Register

| # | Risk | Impact | Likelihood | Mitigation |
|---|------|--------|------------|------------|
| 1 | Circular dependencies between generators and existing passes | Build failure / runtime crashes | Medium | Enforce strict include‑path ordering; run static analysis after each PR |
| 2 | Incomplete test coverage for newly extracted generators | Regression bugs | High | Mandatory unit tests per generator (≥ 80 % line coverage) before merging |
| 3 | Migration from `runtime_error` to `JLM_UNREACHABLE` causing silent failures | Hard‑to‑debug crashes in production | Low | Keep migration flag and run full test suite after each batch of generators |
| 4 | Local verification not run before changes | Regressions merged locally | Medium | Run `./scripts/verify-local.sh` before each commit (local alternative to CI) |
| 5 | Documentation drift (plan vs code) | Confusion for reviewers | Low | Add documentation generation step to CI; run `make docs` nightly |
| **6** | **Uninitialized "whoops" JLM_UNREACHABLE calls in existing code** | **Hard-to-debug crashes** | **High** | **Review all existing JLM_UNREACHABLE calls and replace with runtime_error or proper handling before refactoring** |
| **7** | **Missing Makefile.sub updates causing link errors** | **Build failures** | **Medium** | **Add build verification checklist after phase 5; run `make -n` to verify source inclusion** |
| **8** | **FIRRTL semantic equivalence not properly verified** | **Silent hardware changes** | **High** | **Implement normalization strategy in Phase 8; use both string comparison and AST equivalence checking** |

---

### Risk Register Updates - June 14, 2026

Three additional risks were identified during refactoring plan analysis:

**Risk #6: Existing Uninitialized JLM_UNREACHABLE Calls**
- Found 26 instances of `JLM_UNREACHABLE("whoops")` in current codebase
- Risk: These will crash during refactoring if triggered
- Mitigation: Review and replace with runtime_error or proper handling before refactoring

**Risk #7: Makefile.sub Integration Issues**
- IR layer reorganization requires precise Makefile updates
- Risk: Build failures due to missing source file registration
- Mitigation: Use dependency grouping in Makefile.sub; verify with `make -n`

**Risk #8: FIRRTL Semantic Equivalence Gaps**
- String comparison alone may not detect all equivalent circuits
- Risk: Refactoring could produce different but functionally identical hardware
- Mitigation: Implement normalization + AST equivalence checking in Phase 8

---

## ✅ Consolidated Checklist
### Phase 0 – Infrastructure
- [ ] Create **00_INFRASTRUCTURE.md** describing:
  - `OperationGenerator` interface with runtime_error migration strategy (no JLM_UNREACHABLE until all ops implemented)
  - Thread‑safe `GeneratorRegistry`
  - Local verification script integration (`verify-baseline.sh`, `verify-local.sh`)
- [ ] Add skeleton files:
  - `jlm/hls/backend/rhls2firrtl/generators/GeneratorInterface.hpp`
  - `jlm/hls/backend/rhls2firrtl/generators/GeneratorRegistry.{hpp,cpp}`

### Phase 1 – Setup & Verification
- [ ] Run local verification baseline: `./scripts/verify-baseline.sh`
- [ ] Record test results for comparison (`make check`, `./build/run-libhls-tests`)
- [ ] Capture compilation warnings
- [ ] Commit a **pre‑refactor** git branch (`pre-refactor-state-base`)

### Phase 2 – rvsdg2rhls Refactoring
- [ ] Create `passes/` subdirectories (optimization, conversion, buffering, verification)
- [ ] Move each existing transformation file into its new location
- [ ] Update includes in `rvsdg2rhls.cpp`
- [ ] Add unit tests for every transformed pass

### Phase 3 – rhls2firrtl Refactoring
- [ ] Implement generator directory structure (`generators/` + `builder/`)
- [ ] Extract operation‑specific logic into generators (Arithmetic, Comparison, Memory, Control‑flow, Structural)
- [ ] Register all generators in `GeneratorRegistry`
- [ ] Refactor `RhlsToFirrtlConverter.cpp` to use the registry
- [ ] Add comprehensive tests per generator and integration tests for the full pipeline

### Phase 4 – Optimization Passes
- [ ] Keep existing CNE, IOBarrierRemoval, IOStateElimination files unchanged but add extensive unit tests.
- [ ] Document that FIXME comments are intentional placeholders for future work.
- [ ] Verify no regression after any optimisation pass changes.

### Phase 5 – IR Layer Refactoring
- [ ] Split `hls.hpp` into logical sub‑modules (types, operations, nodes)
- [ ] Update all includes throughout the HLS backend
- [ ] Add documentation for each new header in `docs/DEVELOPMENT_GUIDELINES.md`

### Phase 6 – Test Infrastructure
- [ ] Create common test fixtures under `jlm/hls/backend/tests/fixtures`
- [ ] Ensure every transformation and generator has a dedicated unit test
- [ ] Run local coverage: `make test_coverage`

### Phase 7 – Local Verification & PR Guidelines
- [ ] Create local Makefile.sub checker script (`./scripts/check-makefiles.sh`)
- [ ] Run `./scripts/verify-local.sh` before commits (local alternative to CI)
- [ ] Enforce <400 lines per PR with local lint step
- [ ] Add checklist template to each PR

### Phase 8 – End‑to‑End Validation
- [ ] Write full‑pipeline regression tests (RVSDG → FIRRTL → Verilog → Simulated)
- [ ] Store baseline FIRRTL and Verilog outputs for diffing

### Phase 9 – Pre‑Refactor Benchmarking
- [ ] Capture compile time, memory usage, and test execution time before any changes
- [ ] Store hashes of generated FIRRTL files for later equivalence checks
- [ ] Add a `scripts/benchmark-pre-refactor.sh` helper script

---

## 📄 Next Action
1. **Update** `00_INFRASTRUCTURE.md` with unified error handling strategy (runtime_error migration)
2. **Create** local verification scripts:
   - `./scripts/verify-baseline.sh`
   - `./scripts/verify-local.sh`
3. **Commit** updated plan to the repository

---  

*All references point to files inside `jlm/hls/REFACTORING_PLANS/`. Local testing scripts go in `./scripts/`.*

**Last Updated**: June 14, 2026 - Local testing focus and unified error handling strategy
