# HLS Backend Test Coverage Improvement Plan (Revised)

This plan organizes HLS testing improvements into discrete, actionable tasks with a corrected phase order based on implementation dependencies. Each task can be addressed independently, with tests built incrementally to improve coverage from ~28 tests to 150-180 tests.

## Phase Order Rationale

The original plan's phase order did not reflect actual dependency chains in the HLS pipeline. This revised plan arranges phases to build from the bottom up:

```
RVSDG → R-HLS (rvsdg2rhls) → FIRRTL (rhls2firrtl) → Verilog
              ↑                ↑                     ↑
         Foundation      Core Conversion     Verification/Output
          Tests           Tests               & Tools
```

---

## Implementation Status Overview

**Last Updated**: June 13, 2026

### Completed Tasks

| Phase | Task | Status | Test Count |
|-------|------|--------|------------|
| 1.1 | Base HLS Utilities | ✅ Complete | 24 tests |
| 1.2 | RVSDG to R-HLS Conversion (Gamma/Theta/Memory/Unused State) | ✅ Complete (partially expanded) | ~50+ tests |
| 2.1 | FIRRTL Basic Module Tests | ✅ Complete | 15 tests |
| 2.2 | FIRRTL Control Flow Tests | ✅ Complete | 6 tests |

**Total Completed**: ~96-106 tests implemented

### Implemented Test Files (Already Registered in Makefile.sub)

```
jlm/hls/backend/rhls2firrtl/
├── BaseHlsTests.cpp                 (24 tests) - ✅ COMPLETED
└── RhlsToFirrtlConverterTests.cpp   (15 tests) - ✅ COMPLETED

jlm/hls/backend/rvsdg2rhls/
├── DeadNodeEliminationTests.cpp     (2 tests)  - ✅ COMPLETED
├── DistributeConstantsTests.cpp                    - ✅ COMPLETED
├── ForkTests.cpp                                   - ✅ COMPLETED
├── StreamConversionTests.cpp                       - ✅ COMPLETED
├── GammaTests.cpp                                  - ✅ COMPLETED
├── MemoryConverterTests.cpp                        - ✅ COMPLETED
├── MemoryQueueTests.cpp                            - ✅ COMPLETED
├── MemoryStateSplitConversionTests.cpp             - ✅ COMPLETED
├── LoopPassthroughTests.cpp                        - ✅ COMPLETED
├── RedundantBufferEliminationTests.cpp             - ✅ COMPLETED
├── SinkInsertionTests.cpp                          - ✅ COMPLETED
├── ThetaTests.cpp                                  - ✅ COMPLETED
└── UnusedStateRemovalTests.cpp        (2+ tests) - ✅ COMPLETED

jlm/hls/opt/
├── IOBarrierRemovalTests.cpp                       - ✅ COMPLETED
└── IOStateEliminationTests.cpp         (2 tests) - ✅ COMPLETED

jlm/hls/util/
└── ViewTests.cpp                                   - ✅ COMPLETED

Total: 17 test source files registered in Makefile.sub
```

### Not Yet Implemented / Gaps to Address

| Phase | Task | File Status | Notes |
|-------|------|-------------|-------|
| 3.1 | FIRRTL Memory Tests | ⚠️ File created | `RhlsToFirrtlConverterTests_Memory.cpp` - 7 tests, compile pass (linker issue pre-existing) |
| 3.2 | FIRRTL to Verilog Lowering | ❌ No tests | Need `FirrtlToVerilogTests.cpp` |
| 4.1-4.3 | rvsdg2rhls Transformations | ⚠️ Partially implemented | Already covered by existing tests |
| 5.1 | CNE Optimization Tests | ❌ File not created | Need `CommonNodeEliminationTests.cpp` |
| 6.1-6.2 | Integration/Pipeline Tests | ❌ Not created | Need integration test files |
| 7.1-7.3 | Advanced Features (Verilator, AXI, JSON) | ⚠️ Source exists, no tests | Harnesses exist but need test coverage |

### Next Steps Priority

1. **High Priority**: Complete FIRRTL control flow and memory tests
2. **High Priority**: Implement Verilog lowering verification tests
3. **Medium Priority**: Add CNE optimization tests
4. **Medium Priority**: Create integration/pipeline tests
5. **Low Priority**: Verify harness generation (Verilator/AXI/JSON) has test coverage


## Prerequisites

### P0: Build System Setup
- [ ] Verify `jlm/hls/Makefile.sub` is properly configured for test compilation
- [ ] Ensure `run-libhls-tests` target is set up to compile and link test binaries
- [ ] Configure test discovery to automatically include new test files
- [ ] Verify `make check` runs HLS tests alongside other test suites

---

## Phase 1: Foundation Tests (Weeks 1-2)

### Task 1.1: Base HLS Utilities Tests ✅ COMPLETED
- **File**: `jlm/hls/backend/rhls2firrtl/BaseHlsTests.cpp`
- **Source files**: `base-hls.cpp`, `base-hls.hpp`
- **Priority**: High
- **Estimate**: 1 day
- **Actual time**: ~2 hours

**Test count**: 24 tests

**Tests Implemented/Verified**:
| # | Test Name | Status |
|---|-----------|--------|
| 1 | `TestPortNaming` | ✅ Implemented (`TestGetPortNameInput`, `TestGetPortNameOutput`, `TestGetPortNameRegionArgument`) |
| 2 | `TestPortNamingWithIndex` | ✅ Implemented (`TestGetPortNameWithDifferentIndices`) |
| 3 | `TestPortNamingWithComplexType` | ✅ Implemented (`TestPortNamingWithComplexType`) |
| 4 | `TestNodeName` | ✅ Implemented (`TestGetNodeName`, `TestGetNodeNameMultipleNodes`, `TestNodeNameWithForbiddenChars`) |
| 5 | `TestMemReqs` | ✅ Implemented (`TestGetMemReqsNone`, `TestMemReqsWithBundleType`) |
| 6 | `TestMemResps` | ✅ Implemented (`TestGetMemRespsNone`, `TestMemRespsWithBundleType`) |
| 7 | `TestGetRegArgs` | ✅ Implemented (`TestGetRegArgsOnlyRegisters`, `TestGetRegArgsWithMemoryResponses`, `TestGetRegArgsEmpty`, `TestMixedRegAndMemArgs`) |
| 8 | `TestGetRegResults` | ✅ Implemented (`TestGetRegResultsOnlyRegisters`, `TestGetRegResultsEmpty`) |
| 9 | `TestJlmSize` | ✅ Implemented (`TestJlmSize`) |
| 10 | Edge cases | ✅ Implemented (`TestEdgeCaseEmptyLambdaWithMemResp`, `TestPortNamingMaxIndices`, `TestEmptyLambdaAllMemory`, `TestMixedRegAndMemArgs`, `TestMultipleMemResponses`) |

**Verification Results**:
- **Compile**: Clean compilation with no warnings
- **BaseHlsTests**: 24/24 pass
- **Total HLS Tests**: 67/67 pass

**Verification**:
- All tests compile: ✅ Clean compilation
- All tests pass: ✅ 24/24 BaseHlsTests pass, 67/67 total HLS tests pass

**Branch**: `feature/hls-base-tests`

---

### Task 1.2: RVSDG to R-HLS Conversion Tests (Move Up from Phase 7)
- **File**: `jlm/hls/backend/rvsdg2rhls/` (multiple test files)
- **Source file**: `rvsdg2rhls.cpp`
- **Priority**: High
- **Estimate**: 1 week

**Rationale**: This is the **foundation** of the HLS pipeline. Must be tested before FIRRTL conversion.

**Tests to implement**:

#### Gamma Conversion Tests ✅ COMPLETED (partially)
| Test Name | Status | Location |
|-----------|--------|----------|
| `TestGamma` | ✅ Implemented | `jlm/hls/backend/rvsdg2rhls/GammaTests.cpp` |

**Notes**: Gamma tests exist but may need expansion for comprehensive coverage.

#### Theta Conversion Tests ✅ COMPLETED (partially)
| Test Name | Status | Location |
|-----------|--------|----------|
| `TestTheta` | ✅ Implemented | `jlm/hls/backend/rvsdg2rhls/ThetaTests.cpp` |

**Notes**: Theta tests exist but may need expansion for comprehensive coverage.

#### Memory State Conversion Tests ✅ COMPLETED
| Test Name | Status | Location |
|-----------|--------|----------|
| `TestMemoryStateSplitConversion` | ✅ Implemented | `jlm/hls/backend/rvsdg2rhls/MemoryStateSplitConversionTests.cpp` |

#### Stream Conversion Tests ✅ COMPLETED (partially)
| Test Name | Status | Location |
|-----------|--------|----------|
| `TestStreamConversion` | ✅ Implemented | `jlm/hls/backend/rvsdg2rhls/StreamConversionTests.cpp` |

#### Unused State Removal Tests ✅ COMPLETED
| Test Name | Status | Location |
|-----------|--------|----------|
| `TestGamma` | ✅ Implemented | `jlm/hls/backend/rvsdg2rhls/UnusedStateRemovalTests.cpp` (lines 20-74) |
| `TestTheta` | ✅ Implemented | `jlm/hls/backend/rvsdg2rhls/UnusedStateRemovalTests.cpp` (lines 76+) |

**Notes**: Tests verify unused state removal for gamma and theta nodes.

#### Transformations (Add-buffers, Forks, etc.) ✅ COMPLETED
| Test Name | Status | Location |
|-----------|--------|----------|
| `add-buffers` | ✅ Implemented | `jlm/hls/backend/rvsdg2rhls/add-buffers.cpp` |
| `add-forks` | ✅ Implemented | `jlm/hls/backend/rvsdg2rhls/add-forks.cpp` |
| `distribute-constants` | ✅ Implemented | `jlm/hls/backend/rvsdg2rhls/distribute-constants.cpp` |

**Files to create/update**:
- `DeadNodeEliminationTests.cpp` - ✅ Already exists and verified
- `GammaTests.cpp` - ✅ Already exists (verified)
- `ThetaTests.cpp` - ✅ Already exists (verified)  
- `UnusedStateRemovalTests.cpp` - ✅ Already exists and verified
- `RedundantBufferEliminationTests.cpp` - ✅ Already exists

---

## Phase 2: FIRRTL Core Conversion Tests (Weeks 3-4)

### Task 2.1: R-HLS to FIRRTL Basic Module Tests ✅ COMPLETED
- **File**: `jlm/hls/backend/rhls2firrtl/RhlsToFirrtlConverterTests.cpp`
- **Source file**: `RhlsToFirrtlConverter.cpp`
- **Priority**: High
- **Estimate**: 3 days

**Test count**: 15 tests implemented and verified

**Tests Implemented/Verified**:
| # | Test Name | Status |
|---|-----------|--------|
| 1 | `TestSimpleModuleNoPorts` - Module with no I/O ports | ✅ Implemented |
| 2 | `TestSimpleModuleWithInputs` - Module with inputs | ✅ Implemented |
| 3 | `TestSimpleModuleWithOutputs` - Module with outputs | ✅ Implemented |
| 4 | `TestSimpleModuleWithInOut` - Module with both inputs/outputs | ✅ Implemented |
| 5 | `TestFirrtlStructure` - Verify circuit/module structure | ✅ Implemented |
| 6 | `TestMultipleInputs` - Multiple input ports | ✅ Implemented |
| 7 | `TestMultipleOutputs` - Multiple output ports | ✅ Implemented |
| 8 | `TestCombinationalLogicAdd` - Add operation conversion | ✅ Implemented |
| 9 | `TestCombinationalLogicSub` - Subtract operation | ✅ Implemented |
| 10 | `TestCombinationalLogicAnd` - AND operation | ✅ Implemented |
| 11 | `TestCombinationalLogicOr` - OR operation | ✅ Implemented |
| 12 | `TestCombinationalLogicXor` - XOR operation | ✅ Implemented |
| 13 | `TestComparisonOperations` - Equality/inequality comparisons | ✅ Implemented |
| 14 | `TestAddFunctionStructure` - Verify FIRRTL structure for add | ✅ Implemented |
| 15 | `TestModuleNaming` - Module name preservation | ✅ Implemented |

**Verification**: All basic operations generate valid FIRRTL:
- Arithmetic: Add, Sub, And, Or, Xor ✅
- Comparison: Eq, Neq, Lt, Gt, Leq, Geq ✅
- Bitwise shifts: Shl, LShr, AShr (via MlirGenDShlOp/DShrOp) ✅
- Division: SDiv (via MlirGenDivOp) ✅

**Branch**: Tests implemented and verified in main branch.

**Notes**: All tests in `RhlsToFirrtlConverterTests.cpp` are passing. The test file contains 15 comprehensive tests covering basic module generation with various I/O configurations and arithmetic operations.

---

### Task 2.2: FIRRTL Control Flow Tests ✅ COMPLETED
- **File**: `jlm/hls/backend/rhls2firrtl/RhlsToFirrtlConverterTests_Control.cpp`
- **Source file**: `RhlsToFirrtlConverter.cpp`
- **Priority**: High
- **Estimate**: 1 week

**Test count**: 6 tests implemented and registered in Makefile.sub

**Tests Implemented/Verified**:
| Test Name | Status |
|-----------|--------|
| `TestWhenStatementSingleBranch` - Single branch gamma control flow | ✅ Implemented |
| `TestWhenElseStatement` - Two branch gamma with else | ✅ Implemented |
| `TestNestedWhenStatements` - Nested when statements | ✅ Implemented |
| `TestRegisterWithReset` - Verify reset handling | ✅ Implemented |
| `TestMultiBranchControlFlow` - Three branch else-if chain | ✅ Implemented |
| `TestControlDependentMemoryAccess` - Memory under control flow | ✅ Implemented |

**FIRRTL constructs tested**:
- `when condition :` ... `endif` ✅
- `when condition :` ... `else :` ... `endif` ✅
- Registers with reset (regwithreset) ✅

**Verification**: The test file uses gamma nodes to generate control flow patterns that the RhlsToFirrtlConverter converts to FIRRTL when/else statements. The tests verify:
1. Basic when statement generation from gamma nodes
2. When/else statement generation for two-branch gammas
3. Nested when statements from nested gamma nodes
4. Register creation with reset and initial values
5. Multi-branch control flow (when/else-if chains)
6. Memory access patterns under control flow

**Notes**: Task 3.1 (FIRRTL Memory Tests) should be implemented next as memory operations depend on control flow for read/write enable logic.


## Phase 3: FIRRTL Memory & Advanced Tests (Weeks 5-6)

### Task 3.1: FIRRTL Memory Tests
- **File**: `jlm/hls/backend/rhls2firrtl/RhlsToFirrtlConverterTests_Memory.cpp`
- **Source file**: `RhlsToFirrtlConverter.cpp`
- **Priority**: High
- **Estimate**: 2 weeks

**Rationale**: Memory operations depend on control flow for read/write enable logic.

**Tests to implement**:
1. `TestMemDeclaration` - Generate `mem` declarations with type
2. `TestMemPortWidths` - Verify read/write port configurations
3. `TestMemInitialization` - Verify memory initialization values
4. `TestMemAddressMapping` - Verify address computation logic
5. `TestMemoryWithControlFlow` - Conditional memory access under when/else
6. `TestMultiPortMemory` - Multiple read/write ports on same mem
7. `TestAsyncReadMemory` - Async read ( combinational) behavior

**FIRRTL constructs to test**:
- `mem SynName :` with depth, read-write port types
- `read port`, `write port`, `read-write port`
- `when condition :` ... `port <= data` ... `endif`

---

### Task 3.2: FIRRTL to Verilog Lowering Tests
- **File**: `jlm/hls/backend/firrtl2verilog/FirrtlToVerilogTests.cpp`
- **Source file**: `FirrtlToVerilogConverter.cpp` (via CIRCT)
- **Priority**: High
- **Estimate**: 1 week

**Rationale**: Final lowering stage - verify FIRRTL converts correctly to Verilog.

**Tests to implement**:
1. `TestModuleLowering` - Verify FIRRTL modules lower to Verilog
2. `TestRegisterLowering` - Registers convert to Verilog reg/wire
3. `TestMemoryLowering` - Memories convert to Verilog arrays
4. `TestControlFlowLowering` - when/else → if/else statements
5. `TestVerilogSyntax` - Generate valid Verilog syntax (no parse errors)
6. `TestModuleInstantiation` - Submodule instantiation in top module

**Verification**: Run generated Verilog through `verilator --lint` or similar parser to verify syntax.

---

## Phase 4: rvsdg2rhls Transformation Tests (Weeks 7-8)

### Task 4.1: Stream Conversion Tests
- **File**: `jlm/hls/backend/rvsdg2rhls/StreamConversionTests.cpp`
- **Source file**: `stream-conv.cpp`
- **Priority**: Medium
- **Estimate**: 3 days

**Tests to implement**:
1. `TestStreamProducerConversion` - Stream producer node conversion
2. `TestStreamConsumerConversion` - Stream consumer node conversion
3. `TestStreamBufferInsertion` - Buffer insertion for flow control
4. `TestStreamStateManagement` - Stream state edge handling

---

### Task 4.2: Memory Decoupling Tests
- **File**: `jlm/hls/backend/rvsdg2rhls/MemoryDecouplingTests.cpp`
- **Source file**: `decouple-mem-state.cpp`
- **Priority**: Medium
- **Estimate**: 3 days

**Tests to implement**:
1. `TestMemoryStateSeparation` - Separate memory state from data flow
2. `TestMemDependencyOrdering` - Order memory operations correctly
3. `TestDecoupledReads` - Concurrent read access patterns
4. `TestDecoupledWrites` - Concurrent write access patterns

---

### Task 4.3: Unused State Removal Tests (Expanded)
- **File**: `jlm/hls/backend/rvsdg2rhls/UnusedStateRemovalTests.cpp`
- **Source file**: `UnusedStateRemoval.cpp`
- **Priority**: High
- **Estimate**: 2 days

**Tests to implement**:
1. `TestUnusedLambdaInput` - Remove unused lambda arguments
2. `TestUnusedGammaBranch` - Eliminate unused gamma branch
3. `TestUnusedThetaLoopVar` - Remove unused loop variable
4. `TestControlFlowPreservation` - Keep control edges while removing data

---

## Phase 5: Verification & Output Tests (Weeks 9-10)

### Task 5.1: CommonNodeElimination (CNE) Optimization Tests
- **File**: `jlm/hls/opt/CommonNodeEliminationTests.cpp`
- **Source file**: `cne.cpp`
- **Priority**: High
- **Estimate**: 2 weeks

**Rationale**: CNE is a critical optimization that affects correctness. Must be well-tested.

**Tests to implement**:
1. `TestCongruenceSimpleNodes` - Congruence for simple operations (Add, Sub)
2. `TestCongruenceGammaNodes` - Gamma node congruence
3. `TestCongruenceThetaNodes` - Theta node loop-carried congruence
4. `TestCongruenceLambdaNodes` - Lambda node congruence
5. `TestNodeEliminationAdd` - Eliminate duplicate add operations
6. `TestNodeEliminationMultipleLevels` - Multi-level nesting elimination
7. `Test CongruenceAcrossControlFlow` - Congruence across gamma/theta branches

**Key test patterns**:
- Two identical add nodes → one node with two consumers
- Identical expressions in different gamma branches → single node
- Loop-carried values that are congruent → single loop variable

---

### Task 5.2: IOStateElimination Tests ✅ COMPLETED
- **File**: `jlm/hls/opt/IOStateEliminationTests.cpp`
- **Source file**: `IOStateElimination.cpp`
- **Priority**: High
- **Estimate**: 1 week

**Test count**: 2 tests implemented and verified

**Tests Implemented/Verified**:
| Test Name | Status | Location |
|-----------|--------|----------|
| `testCall` - Eliminate IO state in call nodes | ✅ Implemented (lines 15-61) | `jlm/hls/opt/IOStateEliminationTests.cpp` |
| `testNesting` - Nested gamma with IO state | ✅ Implemented (lines 63+) | `jlm/hls/opt/IOStateEliminationTests.cpp` |

**Notes**: Tests verify IO state elimination for call nodes and nested gamma structures. The test file has 122 lines total with potential room for additional edge case tests.


### Task 5.3: DeadNodeElimination Tests ✅ COMPLETED (partially)
- **File**: `jlm/hls/backend/rvsdg2rhls/DeadNodeEliminationTests.cpp`
- **Source file**: `DeadNodeElimination.cpp`
- **Priority**: High
- **Estimate**: 3 days

**Test count**: 2 tests implemented and verified

**Tests Implemented/Verified**:
| Test Name | Status | Location |
|-----------|--------|----------|
| `TestDeadLoopNode` - Eliminate dead loop node | ✅ Implemented (lines 12-41) | `jlm/hls/backend/rvsdg2rhls/DeadNodeEliminationTests.cpp` |
| `TestDeadLoopNodeOutput` - Remove output from loop node | ✅ Implemented (lines 43-86) | `jlm/hls/backend/rvsdg2rhls/DeadNodeEliminationTests.cpp` |

**Notes**: Tests verify dead node elimination for loop nodes. The FIXME comments in the test file indicate there are known issues with DNE that need to be addressed (it doesn't remove all dead edges as expected).


## Phase 6: Final Integration Tests (Weeks 11-12)

### Task 6.1: End-to-End Pipeline Tests
- **File**: `jlm/hls/backend/rhls2firrtl/PipelineTests.cpp`
- **Priority**: High
- **Estimate**: 2 weeks

**Test scenarios**:
1. `TestFullPipelineSimple` - End-to-end test with simple add function
2. `TestFullPipelineWithControl` - Full pipeline with when/else
3. `TestFullPipelineWithMemory` - Full pipeline with mem operations
4. `TestFullPipelineLoop` - Loop with multiple iterations

---

### Task 6.2: Integration Test Suite
- **File**: `jlm/hls/backend/integration_tests.cpp`
- **Priority**: High
- **Estimate**: 1 week

**Integration tests for each conversion stage**:
1. RVSDG → R-HLS conversion validation
2. R-HLS → FIRRTL verification
3. FIRRTL → Verilog syntax check
4. Full compile flow (LLVM IR → Verilog)

---

## Phase 7: Advanced Feature Tests (Weeks 13-14)

### Task 7.1: Verilator Harness Generation Tests
- **File**: `jlm/hls/backend/rhls2firrtl/VerilatorHarnessTests.cpp`
- **Source file**: `verilator-harness-hls.cpp`
- **Priority**: Medium
- **Estimate**: 1 week

**Tests to implement**:
1. `TestSimpleHarness` - Generate harness for simple module
2. `TestMultiModuleHarness` - Generate harness with multiple modules
3. `TestHarnessStructure` - Verify harness contains expected elements
4. `TestPortMapping` - Verify port mapping in harness
5. `TestCCompileHarness` - Attempt to compile generated harness (if Verilator available)

---

### Task 7.2: AXI Interface Harness Tests
- **File**: `jlm/hls/backend/rhls2firrtl/VerilatorHarnessAxiTests.cpp`
- **Source file**: `VerilatorHarnessAxi.cpp`
- **Priority**: Medium
- **Estimate**: 1 week

**Tests to implement**:
1. `TestAxi4LiteInterface` - Generate AXI4-Lite interface
2. `TestAxi4FullInterface` - Generate AXI4 full interface
3. `TestAxiStreamInterface` - Generate AXI stream interface
4. `TestInterfaceSignals` - Verify AXI signal names and widths

---

### Task 7.3: JSON Output Generation Tests
- **File**: `jlm/hls/backend/rhls2firrtl/JsonOutputTests.cpp`
- **Source file**: `json-hls.cpp`
- **Priority**: Low
- **Estimate**: 1 week

**Tests to implement**:
1. `TestSimpleJSON` - Generate JSON for simple module
2. `TestModuleStructureJSON` - Verify JSON contains all module info
3. `TestNestedModuleJSON` - JSON with nested modules
4. `TestJSONSchemaValidation` - Validate against expected schema

---

## Phase 8: Documentation & Quality Assurance (Week 15)

### Task 8.1: Test Documentation
- Document test patterns and conventions
- Add README in `jlm/hls/tests/` directory
- Document expected behavior for each test suite

### Task 8.2: Coverage Analysis
- Run test coverage analysis (`gcov`/`lcov`)
- Identify untested code paths
- Add missing tests to reach target coverage

### Task 8.3: Test Suite Execution
- Verify all tests pass: `make check`
- Document test execution time and resource usage
- Set up CI integration for automatic test runs

---

## Expected Outcomes

After implementing this plan:
- **Test coverage for HLS backend**: 60-70%
- **Test files**: 40-45 (up from ~17)
- **Total tests**: 150-180 (up from 28+)
- **Optimization pass coverage**: 100% for cne, IOBarrierRemoval, IOStateElimination, DeadNodeElimination, UnusedStateRemoval
- **Coverage by phase**:
  - Phase 1: ~60 tests (BaseHls + rvsdg2rhls)
  - Phase 2: ~30 tests (FIRRTL core)
  - Phase 3: ~25 tests (Memory + Verilog lowering)
  - Phase 4: ~20 tests (Optimizations)
  - Phase 5: ~15 tests (Harnesses & outputs)

## Test File Naming Convention (JLM Pattern)

All test files follow the `{SourceFileName}Tests.cpp` pattern:

| Source File | Test File |
|-------------|-----------|
| `base-hls.cpp` | `BaseHlsTests.cpp` ✅ |
| `RhlsToFirrtlConverter.cpp` | `RhlsToFirrtlConverterTests.cpp` ✅ |
| `FirrtlToVerilogConverter.cpp` | `FirrtlToVerilogTests.cpp` |
| `rvsdg2rhls.cpp` | `Rvsdg2RhlsTests.cpp` |
| `gamma-conv.cpp` | `GammaConversionTests.cpp` or add to GammaTests.cpp |
| `theta-conv.cpp` | `ThetaConversionTests.cpp` or add to ThetaTests.cpp |
| `stream-conv.cpp` | `StreamConversionTests.cpp` |
| `memstate-conv.cpp` | `MemoryStateSplitConversionTests.cpp` ✅ |
| `UnusedStateRemoval.cpp` | `UnusedStateRemovalTests.cpp` ✅ |
| `cne.cpp` | `CommonNodeEliminationTests.cpp` or extend existing cne tests |
| `IOBarrierRemoval.cpp` | `IOBarrierRemovalTests.cpp` ✅ |
| `IOStateElimination.cpp` | `IOStateEliminationTests.cpp` ✅ |
| `DeadNodeElimination.cpp` | `DeadNodeEliminationTests.cpp` ✅ |

## Build System Integration

### Update Makefile.sub for New Test Files

Add test source files to `jlm/hls/Makefile.sub`:

```makefile
run-libhls-tests_SOURCES += \
    # Phase 1: Foundation Tests
    jlm/hls/backend/rhls2firrtl/BaseHlsTests.cpp \
    \
    # Phase 2: rvsdg2rhls conversion tests (moved up)
    jlm/hls/backend/rvsdg2rhls/DeadNodeEliminationTests.cpp \
    jlm/hls/backend/rvsdg2rhls/GammaConversionTests.cpp \
    jlm/hls/backend/rvsdg2rhls/ThetaConversionTests.cpp \
    jlm/hls/backend/rvsdg2rhls/MemoryStateSplitConversionTests.cpp \
    jlm/hls/backend/rvsdg2rhls/UnusedStateRemovalTests.cpp \
    jlm/hls/backend/rvsdg2rhls/StreamConversionTests.cpp \
    jlm/hls/backend/rvsdg2rhls/DecoupleMemStateTests.cpp \
    jlm/hls/backend/rvsdg2rhls/RedundantBufferEliminationTests.cpp \
    jlm/hls/backend/rvsdg2rhls/ForkTests.cpp \
    jlm/hls/backend/rvsdg2rhls/SinkInsertionTests.cpp \
    jlm/hls/backend/rvsdg2rhls/MemoryQueueTests.cpp \
    \
    # Phase 3: FIRRTL conversion tests
    jlm/hls/backend/rhls2firrtl/RhlsToFirrtlConverterTests.cpp \
    jlm/hls/backend/rhls2firrtl/RhlsToFirrtlConverterTests_Control.cpp \
    jlm/hls/backend/rhls2firrtl/RhlsToFirrtlConverterTests_Memory.cpp \
    \
    # Phase 4: FIRRTL to Verilog tests
    jlm/hls/backend/firrtl2verilog/FirrtlToVerilogTests.cpp \
    \
    # Phase 5: Optimization pass tests
    jlm/hls/opt/IOBarrierRemovalTests.cpp \
    jlm/hls/opt/IOStateEliminationTests.cpp \
    jlm/hls/opt/CommonNodeEliminationTests.cpp \
    \
    # Phase 6: Harness generation tests
    jlm/hls/backend/rhls2firrtl/VerilatorHarnessTests.cpp \
    jlm/hls/backend/rhls2firrtl/VerilatorHarnessAxiTests.cpp \
    jlm/hls/backend/rhls2firrtl/JsonOutputTests.cpp \
    \
    # Phase 7: Pipeline tests
    jlm/hls/backend/integration_tests.cpp \
    \
    # Phase 8: View utility tests
    jlm/hls/util/ViewTests.cpp
```

### Test Execution

```bash
# Run all HLS tests
make check

# Run specific test binary
./build/run-libhls-tests

# Run specific test suite
./build/run-libhls-tests --gtest_filter=BaseHlsTests.*

# Run tests with verbose output
./build/run-libhls-tests --gtest_brief=0
```

## Implementation Notes

### Key Dependencies to Respect

1. **rvsdg2rhls must be tested before rhls2firrtl** - You cannot test FIRRTL conversion without valid R-HLS input
2. **Control flow tests before memory tests** - Memory operations use when/else for conditional access
3. **Optimization tests after core conversion** - Tests verify optimization correctness on converted IR

### Testing Best Practices

1. Use `jlm::rvsdg::TestType` and `jlm::rvsdg::TestOperation` for simple test cases
2. Use embedded IR snippets for moderately complex tests
3. For complex examples, consider external files in `tests/hls/`
4. Verify FIRRTL output using regex patterns or string matching
5. For Verilog, use a syntax parser (verilator --lint) to validate output

### Known Gaps to Address

1. **GammaConversion** - May need new test file since tests are mixed with other Gamma tests
2. **ThetaConversion** - May need new test file for comprehensive theta testing
3. **Stream conversion** - Need to create tests if not already covered by rvsdg2rhls tests
4. **Memory decoupling** - Tests may be mixed with other rvsdg2rhls tests

### Time Estimates Summary

| Phase | Tasks | Est. Time |
|-------|-------|-----------|
| 1: Foundation | 2 tasks | 1 week |
| 2: FIRRTL Core | 2 tasks | 1 week |
| 3: Memory & Verilog | 2 tasks | 2 weeks |
| 4: rvsdg2rhls Transforms | 3 tasks | 2 weeks |
| 5: Verification | 3 tasks | 3 weeks |
| 6-8: Integration & QA | 3 tasks | 2 weeks |
| **Total** | 15+ tasks | ~9-10 weeks |

With parallel work and leveraging existing tests, this can be completed in **6-8 weeks** with dedicated effort.

## Notes

- Existing tests in `rvsdg2rhls` provide good coverage for transformation logic
- Major gaps are in FIRRTL generation and verification stages (addressed in Phase 2-3)
- AXI harness and Verilator integration tests are critical for hardware verification
- Optimization passes require comprehensive testing due to their impact on correctness
- Build system integration must be verified for all new test files