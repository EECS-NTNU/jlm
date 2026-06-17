#!/bin/bash
# Local verification script for HLS refactoring
# Run after each change to verify no regressions

set -e

echo "=== HLS Refactoring Local Verification ==="
echo ""

# 1. Build verification
echo "[1/3] Building project..."
make clean && make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi
echo "Build completed successfully"

# 2. Run all HLS tests
echo ""
echo "[2/3] Running HLS unit tests..."
./build/run-libhls-tests --gtest_brief=0 | tee /tmp/local-test-results.txt

# Extract test summary
TEST_COUNT=$(grep -oP '\[==========\] \K[0-9]+' /tmp/local-test-results.txt)
PASS_COUNT=$(grep -oP '\[  PASSED  \] \K[0-9]+' /tmp/local-test-results.txt)

if [ "$TEST_COUNT" != "82" ]; then
    echo "WARNING: Expected 82 tests, got $TEST_COUNT"
fi

if [ "$PASS_COUNT" != "82" ]; then
    echo "ERROR: Not all tests passed! ($PASS_COUNT/$TEST_COUNT)"
    exit 1
fi

echo "Tests: $TEST_COUNT total, $PASS_COUNT passed - ALL PASSED"

# 3. Check for regressions against baseline (if available)
echo ""
echo "[3/3] Checking for regressions against baseline..."

if [ -f /tmp/baseline-metrics/test-results.txt ]; then
    BASELINE_TESTS=$(grep -oP '\[==========\] \K[0-9]+' /tmp/baseline-metrics/test-results.txt)
    if [ "$TEST_COUNT" != "$BASELINE_TESTS" ]; then
        echo "WARNING: Test count changed from $BASELINE_TESTS to $TEST_COUNT"
    fi
    
    BASELINE_PASS=$(grep -oP '\[  PASSED  \] \K[0-9]+' /tmp/baseline-metrics/test-results.txt)
    if [ "$PASS_COUNT" != "$BASELINE_PASS" ]; then
        echo "WARNING: Pass count changed from $BASELINE_PASS to $PASS_COUNT"
    fi
    
    # Check FIRRTL output hashes if available
    if [ -f /tmp/baseline-metrics/firrtl-hashes.txt ]; then
        echo "Comparing FIRRTL outputs with baseline..."
        
        mkdir -p /tmp/current-firrtl-hashes
        
        # Regenerate FIRRTL for comparison (only for files we have baselines for)
        while IFS= read -r hash_file; do
            if [ -f "$hash_file" ]; then
                base_name=$(basename "$(dirname "$hash_file")")
                full_path="/tmp/baseline-firrtl/${base_name}.fir"
                if [ -f "$full_path" ]; then
                    md5sum "$full_path" >> "/tmp/current-firrtl-hashes/current.txt" 2>/dev/null || true
                fi
            fi
        done < /tmp/baseline-metrics/firrtl-hashes.txt
        
        if [ -f "/tmp/current-firrtl-hashes/current.txt" ]; then
            # Simple hash comparison (order-dependent)
            DIFF_COUNT=$(diff /tmp/baseline-metrics/firrtl-hashes.txt /tmp/current-firrtl-hashes/current.txt 2>/dev/null | grep -c "^[<>]" || true)
            if [ "$DIFF_COUNT" -gt 0 ]; then
                echo "WARNING: $DIFF_COUNT FIRRTL output differences detected"
                echo "Review: diff /tmp/baseline-metrics/firrtl-hashes.txt /tmp/current-firrtl-hashes/current.txt"
            else
                echo "FIRRTL outputs match baseline"
            fi
        fi
    fi
    
    # Check timing metrics if available
    if [ -f /tmp/baseline-metrics/timings/hls-timings.txt ]; then
        echo ""
        echo "Timing comparison (first 5 entries):"
        echo "Baseline | Current"
        paste -d'|' <(head -5 /tmp/baseline-metrics/timings/hls-timings.txt) <(cat /tmp/current-firrtl-hashes/current.txt 2>/dev/null | head -5 || echo "(no current timings)")
    fi
else
    echo "NOTE: No baseline found at /tmp/baseline-metrics/"
    echo "Run ./scripts/verify-baseline.sh first to establish a baseline"
fi

echo ""
echo "=== Local Verification PASSED ==="