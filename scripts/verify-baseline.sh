#!/bin/bash
# Baseline verification script for HLS refactoring
# Captures baseline metrics before any changes are made

set -e

echo "=== HLS Refactoring Baseline Capture ==="
echo ""

# 1. Build verification
echo "[1/5] Building project..."
make clean && make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi
echo "Build completed successfully"

# 2. Run all HLS tests and save results
echo ""
echo "[2/5] Running HLS unit tests..."
mkdir -p /tmp/baseline-metrics
./build/run-libhls-tests --gtest_brief=0 | tee /tmp/baseline-metrics/test-results.txt

# Extract test summary
TEST_COUNT=$(grep -oP '\[==========\] \K[0-9]+' /tmp/baseline-metrics/test-results.txt)
PASS_COUNT=$(grep -oP '\[  PASSED  \] \K[0-9]+' /tmp/baseline-metrics/test-results.txt)
echo "Tests: $TEST_COUNT total, $PASS_COUNT passed"

# 3. Generate FIRRTL output and store hashes for equivalence checks
echo ""
echo "[3/5] Generating baseline FIRRTL output..."
mkdir -p /tmp/baseline-firrtl

# Find test LLVM files if they exist in tests directory
if [ -d "tests" ]; then
    for ll_file in tests/*.ll 2>/dev/null; do
        if [ -f "$ll_file" ]; then
            base_name=$(basename "$ll_file" .ll)
            echo "  Processing $base_name..."
            ./build/tools/jlm-hls/jlm-hls.la --output-format=firrtl "$ll_file" > "/tmp/baseline-firrtl/${base_name}.fir" 2>/dev/null || true
        fi
    done
fi

# Generate hashes for all FIRRTL files
if [ -d "/tmp/baseline-firrtl" ] && [ "$(ls -A /tmp/baseline-firrtl/*.fir 2>/dev/null)" ]; then
    cd /tmp/baseline-firrtl
    md5sum *.fir > /tmp/baseline-metrics/firrtl-hashes.txt
    cd -
    echo "Generated FIRRTL hashes"
else
    echo "No FIRRTL files generated (expected if no test LLVM files available)"
fi

# 4. Store timing metrics
echo ""
echo "[4/5] Capturing timing metrics..."
mkdir -p /tmp/baseline-metrics/timings

# Benchmark a few test cases for timing
for ll_file in tests/*.ll 2>/dev/null; do
    if [ -f "$ll_file" ]; then
        base_name=$(basename "$ll_file" .ll)
        start_time=$(date +%s%N)
        ./build/tools/jlm-hls/jlm-hls.la --output-format=firrtl "$ll_file" > /dev/null 2>&1 || true
        end_time=$(date +%s%N)
        duration=$(( (end_time - start_time) / 1000000 ))
        echo "${base_name}: ${duration}ms" >> /tmp/baseline-metrics/timings/hls-timings.txt
    fi
done

# 5. Capture compilation metrics
echo ""
echo "[5/5] Capturing compilation statistics..."
make -n > /tmp/baseline-metrics/build-plan.txt 2>&1 || true
wc -l jlm/hls/**/*.cpp jlm/hls/**/*.hpp 2>/dev/null | tail -1 >> /tmp/baseline-metrics/source-stats.txt

echo ""
echo "=== Baseline Capture Complete ==="
echo "Results saved to: /tmp/baseline-metrics/"
cat /tmp/baseline-metrics/test-results.txt | tail -5