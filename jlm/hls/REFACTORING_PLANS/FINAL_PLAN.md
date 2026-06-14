# HLS Backend Refactoring – Final Consolidated Plan

## 📋 Summary Table
| Phase | Document | Priority | Estimated Duration | Status |
|------|----------|----------|-------------------|--------|
| 0️⃣ **Infrastructure** | [00_INFRASTRUCTURE.md](00_INFRASTRUCTURE.md) | CRITICAL | 2 days | Planned |
| 1️⃣ **Setup & Verification** | [01_SETUP_VERIFICATION.md](01_SETUP_VERIFICATION.md) | CRITICAL | 3‑5 days | In Progress |
| 2️⃣ **RVSDG → RHLS Refactoring** | [02_RVSDG2RHLS_REFACTORING.md](02_RVSDG2RHLS_REFACTORING.md) | HIGH | 1‑2 weeks | Planned |
| 3️⃣ **RHLS → FIRRTL Refactoring** | [03_RHLS2FIRRTL_REFACTORING.md](03_RHLS2FIRRTL_REFACTORING.md) | HIGH | 2‑3 weeks | Planned |
| 4️⃣ **Optimization Passes** | [04_OPTIMIZATION_PASSES.md](04_OPTIMIZATION_PASSES.md) | MEDIUM | 1 week | Planning |
| 5️⃣ **IR Layer Refactoring** | [05_IR_LAYER_REFACTORING.md](05_IR_LAYER_REFACTORING.md) | MEDIUM | 1 week | Planned |
| 6️⃣ **Test Infrastructure** | [06_TEST_INFRASTRUCTURE.md](06_TEST_INFRASTRUCTURE.md) | CRITICAL | Ongoing | In Progress |
| 7️⃣ **PR Guidelines & CI** | [07_PR_GUIDELINES.md](07_PR_GUIDELINES.md) | HIGH | Ongoing | In Progress |
| 8️⃣ **End‑to‑End Validation** | [08_E2E_VALIDATION.md](08_E2E_VALIDATION.md) | HIGH | Ongoing | Planned |
| 9️⃣ **Pre‑Refactor Benchmarking** | [09_PRE_REFACTOR_BENCHMARKING.md](09_PRE_REFACTOR_BENCHMARKING.md) | CRITICAL | 3 days | Planned |

*All phases are limited to modifications **only inside `jlm/hls/`**.*

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
| 4 | CI pipeline not updated with new source files | Missing builds on PRs | Medium | Update `.github/workflows/hls_refactor_ci.yml` in Phase 7; verify with dry‑run builds |
| 5 | Documentation drift (plan vs code) | Confusion for reviewers | Low | Add documentation generation step to CI; run `make docs` nightly |

---

## ✅ Consolidated Checklist
### Phase 0 – Infrastructure
- [ ] Create **00_INFRASTRUCTURE.md** describing:
  - `OperationGenerator` interface
  - Thread‑safe `GeneratorRegistry`
  - Migration strategy (runtime_error → `JLM_UNREACHABLE`)
- [ ] Add skeleton files:
  - `jlm/hls/backend/rhls2firrtl/generators/GeneratorInterface.hpp`
  - `jlm/hls/backend/rhls2firrtl/generators/GeneratorRegistry.{hpp,cpp}`

### Phase 1 – Setup & Verification
- [ ] Record baseline test results (`make check`, `run-libhls-tests`)
- [ ] Capture compilation warnings
- [ ] Commit a **pre‑refactor** git branch (`pre-refactor-state-base`)

### Phase 2 – RVSDG → RHLS Refactoring
- [ ] Create `passes/` subdirectories (optimization, conversion, buffering, verification)
- [ ] Move each existing transformation file into its new location
- [ ] Update includes in `rvsdg2rhls.cpp`
- [ ] Add unit tests for every transformed pass

### Phase 3 – RHLS → FIRRTL Refactoring
- [ ] Implement generator directory structure (`generators/` + `builder/`)
- [ ] Extract operation‑specific logic into generators (Arithmetic, Comparison, Memory, Control‑flow, Structural)
- [ ] Register all generators in `GeneratorRegistry`
- [ ] Refactor `RhlsToFirrtlConverter.cpp` to use the registry
- [ ] Add comprehensive tests per generator and integration tests for the full pipeline

### Phase 4 – Optimization Passes
- [ ] Keep existing CNE, IOBarrierRemoval, IOStateElimination files unchanged but add extensive unit tests
- [ ] Document that FIXME comments are intentional placeholders for future generalisation
- [ ] Verify no regression after any optimisation pass changes

### Phase 5 – IR Layer Refactoring
- [ ] Split `hls.hpp` into logical sub‑modules (types, operations, nodes)
- [ ] Update all includes throughout the HLS backend
- [ ] Add documentation for each new header in `docs/DEVELOPMENT_GUIDELINES.md`

### Phase 6 – Test Infrastructure
- [ ] Create common test fixtures under `jlm/hls/backend/tests/fixtures`
- [ ] Ensure every transformation and generator has a dedicated unit test
- [ ] Integrate coverage reporting (`gcov` / `llvm-cov`) into CI

### Phase 7 – PR Guidelines & CI
- [ ] Update `.github/workflows/hls_refactor_ci.yml` to compile new files
- [ ] Enforce <400 lines per PR (CI lint step)
- [ ] Add checklist template to each PR (auto‑generated by the plan)

### Phase 8 – End‑to‑End Validation
- [ ] Write full‑pipeline regression tests (RVSDG → FIRRTL → Verilog → Simulated)
- [ ] Store baseline FIRRTL and Verilog outputs for diffing

### Phase 9 – Pre‑Refactor Benchmarking
- [ ] Capture compile time, memory usage, and test execution time before any changes
- [ ] Store hashes of generated FIRRTL files for later equivalence checks
- [ ] Add a `scripts/benchmark-pre-refactor.sh` helper script

---

## 📄 Next Action
1. **Create** `00_INFRASTRUCTURE.md` (Phase 0) – see the *Infrastructure* section above.  
2. **Commit** this `FINAL_PLAN.md` to the repository.  

Once these two items are merged, all subsequent phases can proceed directly from a single source of truth.

---  

*All references point to files inside `jlm/hls/REFACTORING_PLANS/`. No modifications outside that directory are required.*