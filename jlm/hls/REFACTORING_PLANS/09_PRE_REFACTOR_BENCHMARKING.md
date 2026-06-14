# Phase 9: Pre-Refactoring Benchmarking and Verification Baseline

## Document Information
- **Phase**: 9 (Pre-Refactoring Baseline)
- **Priority**: CRITICAL - Must be completed before any refactoring begins
- **Status**: New Phase Added
- **Last Updated**: June 14, 2026

---

## 1. Purpose and Scope

This phase establishes a comprehensive verification baseline for the HLS backend refactoring. All measurements captured here serve as the "before" state against which all subsequent refactored code will be validated.

### Why This Phase Is Critical
- **Regression Detection**: Without a baseline, performance regressions are invisible
- **Verification Anchor**: All functional equivalence checks reference this state
- **Risk Mitigation**: Pre-refactoring validation prevents broken commits

---

## 2. Baseline Capture Procedure

### Prerequisites
```bash
# Ensure clean build with no refactored code yet
make clean && make -j$(nproc) 2>&1 | tee /tmp/pre-build.log

# Verify build succeeded
grep -i "error" /tmp/pre-build.log || echo "Build successful"
```

### 2.1 Unit Test Baseline

```bash
# Run all HLS unit tests and save results
./build/run-libhls-tests --gtest_brief=0 > /tmp/unit-baseline.txt 2>&1

# Extract summary for storage
grep -E "(PASSED|FAILED)" /tmp/unit-baseline.txt | tail -5
```

### 2.2 External Test Suite Baseline (run-hls-test.sh)

```bash
# Generate golden cycle counts before refactoring
export BENCHMARK_DIR=/tmp/hls-benchmark-base
./scripts/run-hls-test.sh --benchmark-path $BENCHMARK_DIR --parallel 16

# Save the full test output for comparison later
cp -r /tmp/hls-benchmark-base/build /tmp/benchmark-base-build/

# Store golden values separately
cp -r .github/golden/hls-test-suite /tmp/golden-baseline/
```

### 2.3 FIRRTL Output Baseline

```bash
# Generate FIRRTL for a representative set of kernels
mkdir -p /tmp/firrtl-baseline

for kernel in tests/hls/*.ll; do
    jlm-hls "$kernel" > "/tmp/firrtl-baseline/$(basename "$kernel" .ll).fir"
done

# Store baseline hashes
cd /tmp/firrtl-baseline
md5sum *.fir > firrtl-md5.txt
```

### 2.4 Performance Metrics Baseline

```bash
# Measure compilation pipeline timing
mkdir -p /tmp/perf-baseline

for kernel in tests/hls/*.ll; do
    start=$(date +%s%N)
    jlm-hls "$kernel" > /dev/null
    end=$(date +%s%N)
    
    duration=$(( (end - start) / 1000000 ))  # Convert to milliseconds
    echo "$(basename $kernel .ll): ${duration}ms" >> /tmp/perf-baseline/timings.txt
done

# Calculate statistics
awk '{sum+=$2; count++} END {print "Average compilation time:", sum/count "ms"}' \
    /tmp/perf-baseline/timings.txt > /tmp/perf-baseline/stats.txt
```

### 2.5 Code Coverage Baseline

```bash
# Build with coverage instrumentation (if supported by toolchain)
make clean && make CXXFLAGS="--coverage" LDFLAGS="--coverage"

# Run tests to generate coverage data
./build/run-libhls-tests

# Generate coverage report
lcov --capture --directory . --output-file /tmp/coverage-baseline.info
genhtml /tmp/coverage-baseline.info --output-directory /tmp/coverage-report-base
```

---

## 3. Verification Checklist (Pre-Refactoring)

Before any refactoring work begins, verify:

| Check | Command | Pass Criteria |
|-------|---------|---------------|
| Unit tests pass | `./build/run-libhls-tests` | 100% tests pass |
| External suite passes | `./scripts/run-hls-test.sh --parallel N` | All kernels match golden |
| FIRRTL generated | `jlm-hls input.ll > output.fir` | No errors, valid FIRRTL |
| Verilator works | `verilator --version` | Available and functional |
| Coverage data captured | `lcov` report exists | Coverage baseline saved |

---

## 4. Pre-Refactoring Sign-Off

### Required Artifacts

Each refactoring phase requires these pre-refactoring signatures:

```
[ ] Phase X baseline captured at: /tmp/phase-x-baseline-$(date +%Y%m%d)
[ ] Unit test results stored in: /tmp/unit-baseline-phaseX.txt
[ ] External test suite results in: /tmp/benchmark-base-build/
[ ] FIRRTL baseline hashes in: /tmp/firrtl-baseline/md5sum.txt
[ ] Performance baseline in: /tmp/perf-baseline/timings.txt
```

### Pre-Refactoring Checklist

**For each PR that modifies HLS backend code:**

1. **Build Verification**
   ```bash
   make clean && make -j$(nproc)
   ./build/run-libhls-tests --gtest_filter=*.*
   ```

2. **Functional Equivalence (must match pre-refactor)**
   ```bash
   # Run test suite and compare against golden baseline
   ./scripts/run-hls-test.sh --parallel 16
   
   # Check for cycle count differences
   grep -E "(golden|cycle)" /tmp/benchmark-base-build/*.hls.log
   ```

3. **Performance Threshold Check**
   ```bash
   # Compilation should not be >50% slower than baseline
   # (see Phase 9.6 for exact thresholds)
   ```

4. **Output Verification**
   ```bash
   # Regenerate FIRRTL and compare to baseline hashes
   cd /tmp/firrtl-baseline
   md5sum *.fir | diff - firrtl-md5.txt
   ```

---

## 5. Performance Thresholds

### Compilation Time Budget

| Refactoring Type | Max Allowable Slowdown | Rationale |
|------------------|----------------------|-----------|
| Code organization (file splits) | ≤20% | Minimal overhead expected |
| New abstraction layers | ≤30% | Virtual calls add cost |
| Generator pattern extraction | ≤50% | Registry lookups, indirection |

### Formula

```cpp
// Pre-refactoring baseline: T_pre (from Phase 9.4)
// Post-refactoring measurement: T_post
// Threshold multiplier: M (see table above)

// Pass if:
T_post ≤ T_pre × (1 + M)

// Example for generator pattern extraction (M = 0.5):
// If baseline = 100ms, max allowed = 150ms
```

### Measurement Method

```bash
# Automated performance verification script
#!/bin/bash
# scripts/verify-performance.sh

BASELINE_DIR=${1:-/tmp/perf-baseline}
CURRENT_RESULTS=/tmp/perf-current.txt

# Run current measurements
for kernel in tests/hls/*.ll; do
    start=$(date +%s%N)
    jlm-hls "$kernel" > /dev/null
    end=$(date +%s%N)
    echo "$(basename $kernel .ll): $(( (end - start) / 1000000 ))ms"
done > $CURRENT_RESULTS

# Compare with baseline
echo "Performance comparison:"
paste $BASELINE_DIR/timings.txt $CURRENT_RESULTS | \
  awk '{printf "%-30s %8s %8s %s\n", $1, $2, $4, 
        ($4/$2 > 1.5 ? "REGRESSION!" : "OK")}'
```

---

## 6. Regression Detection Thresholds

### Unit Test Performance

| Metric | Baseline | Threshold | Action if Exceeded |
|--------|----------|-----------|-------------------|
| Total test execution time | T0 | 1.5×T0 | Investigate slowdown |
| Single test max duration | T_max | 2×T_max | Check for hangs |
| Memory per test suite | M0 | 2×M0 | Check for leaks |

### External Test Suite Performance

| Metric | Baseline | Threshold | Action if Exceeded |
|--------|----------|-----------|-------------------|
| Verilator compilation time | T0 | 1.5×T0 | Check Verilog size |
| Simulation execution time | T0 | 1.2×T0 | Check for algorithmic issues |
| Total test suite runtime | T0 | 1.3×T0 | Investigate root cause |

---

## 7. Documentation Requirements

### Pre-Refactoring Summary Document

Each baseline capture must include:

```
# Pre-Refactoring Baseline - Phase X
Date: $(date +%Y-%m-%d)
Commit: $(git rev-parse HEAD)

## Unit Tests
- Total tests: X
- Pass rate: 100%
- Execution time: Y ms

## External Test Suite
- Kernels tested: N
- Golden match rate: 100%
- Average cycles difference: ±X

## Performance Metrics
- Average compilation time: X ms
- Max compilation time: X ms
- Memory usage: X MB

## Code Coverage
- Overall coverage: X%
- Uncovered areas: [list]

## Baseline Storage Locations
- Unit tests: /tmp/unit-baseline-phaseX.txt
- External suite: /tmp/benchmark-base-build/
- FIRRTL hashes: /tmp/firrtl-baseline/md5sum.txt
- Performance: /tmp/perf-baseline/timings.txt
- Coverage: /tmp/coverage-report-base/
```

---

## 8. Rollback Verification

If refactored code introduces regressions:

### Immediate Actions

1. **Stop** - Do not merge regression-inducing changes
2. **Compare** - Use baseline data to identify what changed
3. **Diagnose** - Determine if regression is acceptable (within threshold) or unacceptable (>threshold)

### Acceptable Regressions

- Performance: Within 50% budget for generator pattern phase
- Functional: Equivalent output (same FIRRTL/Verilog)
- Size: Reasonable code size increase (<2× for refactoring overhead)

### Unacceptable Regressions

- **Any** test that was passing now fails
- Performance degradation >50% without justification
- Functional equivalence violations (different FIRRTL output)
- Memory leaks detected

---

## 9. CI/CD Integration

### Pre-Submit Hook

```bash
#!/bin/bash
# .github/pre-submit-check.sh

echo "=== Pre-Refactoring Baseline Verification ==="

# Verify baseline exists
if [ ! -f /tmp/firrtl-baseline/md5sum.txt ]; then
    echo "ERROR: No baseline found. Run Phase 9 first."
    exit 1
fi

# Run tests
./build/run-libhls-tests --gtest_brief=0 || { echo "Unit tests failed"; exit 1; }

# Verify functional equivalence
cd /tmp/firrtl-baseline
md5sum *.fir | diff - firrtl-md5.txt || {
    echo "ERROR: FIRRTL output changed (regression!)"
    exit 1
}

echo "=== Pre-Refactoring Verification PASSED ==="
```

---

## 10. Success Criteria

### Phase 9 Completion Checklist

| Criterion | Status |
|-----------|--------|
| Unit test baseline captured | ✅/❌ |
| External suite golden values stored | ✅/❌ |
| FIRRTL output hashes recorded | ✅/❌ |
| Performance metrics documented | ✅/❌ |
| Pre-refactor verification script created | ✅/❌ |

### Deliverables

1. **Baseline directory structure**
   ```
   /tmp/pre-ref-baseline-$(date)/
   ├── unit-tests.txt
   ├── benchmark-results/
   │   └── *.hls.log
   ├── golden-values/
   │   └── *.cycles
   ├── firrtl-hashes/
   │   └── md5sum.txt
   └── perf-metrics/
       ├── timings.txt
       └── stats.txt
   ```

2. **Pre-refactor sign-off template** for PR descriptions

3. **Verification script**: `scripts/verify-baseline.sh`

---

## 11. Next Steps After Phase 9

### When Baseline Is Captured

1. Begin Phase 0 (Generator Interface)
2. Each PR must include pre/post comparison
3. Update baseline after major refactoring milestones

### Baseline Updates

After significant refactoring phases, update the baseline:

```bash
# After completing a major phase, update baseline if results are acceptable:
mv /tmp/firrtl-baseline/md5sum.txt /tmp/firrtl-baseline/md5sum-phase2.txt
```

---

*This phase ensures every refactoring change is measured against a known-good baseline. No code should be merged without Phase 9 verification completed first.*

Version 1.0: Added June 14, 2026 as prerequisite for all HLS backend refactoring.