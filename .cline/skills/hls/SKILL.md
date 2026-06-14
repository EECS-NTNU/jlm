--- 
## File Header Template

```cpp
/*
 * Copyright YEAR Author <email@example.com>
 * See COPYING for terms of redistribution.
 */

#ifndef PROJECT_MODULE_FILENAME_HPP
#define PROJECT_MODULE_FILENAME_HPP

// Include statements, namespace declarations, class definitions, etc.

#endif // PROJECT_MODULE_FILENAME_HPP
```
name: hls
description: JLM compiler HLS mode. Use when working with high-level synthesis, converting LLVM IR to FIRRTL, or generating Verilog for hardware.
---

## Quick One‑Liner Pipeline
- Compile C → LLVM IR: `jlc -emit-llvm src.c -o src.bc`
- Optimize (optional): `jlm-opt src.bc -o src.opt.bc`
- Generate FIRRTL: `jhls --emit-firrtl src.bc -o design.fir`
- Convert to Verilog: `jhls --emit-verilog src.bc -o design.v`

-------

The High-Level Synthesis (HLS) mode transforms LLVM IR code into hardware descriptions. It uses the LLVM frontend to create an RVSDG, then applies HLS-specific transformations to produce FIRRTL (a hardware description language), which can be further lowered to Verilog.

## Tool

### `jhls` - JLM HLS Tool

- **Purpose**: Generate hardware from C code
- **Usage**: `jhls [options] input.c -o output.fir`
- **Process**: LLVM IR → RVSDG → R-HLS → FIRRTL → (Verilog)
- **Location**: `tools/jhls/jhls.cpp`

## Workflow

### Complete Pipeline

```bash
# Compile C to LLVM (using jlc or clang)
clang -S -emit-llvm input.c -o input.ll

# Convert LLVM to FIRRTL (using jhls)
jhls --emit-firrtl input.ll -o output.fir

# Optional: Convert FIRRTL to Verilog
jhls --emit-verilog input.ll -o output.v
````

### Hybrid Pipeline with Optimizations

```bash
# Compile and optimize
jlc -emit-llvm input.c -o input.bc
jlm-opt input.bc -o input.opt.bc

# Convert optimized code to FIRRTL
jhls --emit-firrtl input.opt.bc -o output.fir
````

## Backend Components

The HLS backend is organized in three stages:

### RVSDG to R-HLS

- **Directory**: `jlm/hls/backend/rvsdg2rhls/`
- **Purpose**: Transform RVSDG to R-HLS (Regional HLS representation)
- **Key Transformations**:
  - Memory disambiguation
  - State machine extraction
  - Resource allocation
  - Control flow scheduling

### R-HLS to FIRRTL

- **Directory**: `jlm/hls/backend/rhls2firrtl/`
- **Purpose**: Convert R-HLS to FIRRTL hardware description
- **Key Classes**:
  - `RhlsToFirrtlConverter`: Main conversion orchestrator
  - Various FIRRTL node emitters

### FIRRTL to Verilog

- **Directory**: `jlm/hls/backend/firrtl2verilog/`
- **Purpose**: Lower FIRRTL to Verilog for simulation/implementation
- **Dependencies**: CIRCT for FIRRTL lowering

## R-HLS: Regional HLS Intermediate Representation

R-HLS is a hardware-specific IR that extends RVSDG with hardware constructs:

### Key Concepts

#### Memory Systems

- **Memory Operations**: Load/store with hardware timing
- **Memory Interfaces**: AXI, AHB, etc.
- **Memory Disambiguation**: Static/dynamic analysis for concurrent access

#### Control Flow

- **Finite State Machines**: Explicit state encoding
- **Operation Chaining**: Combining computations for area efficiency
- **Pipeline Stages**: Automatic or manual pipeline insertion

#### Data Flow

- **Register Transfer**: Hardware register allocation
- **Combinational Logic**: Data-dependent combinators
- **Timing Constraints**: Setup/hold time considerations

### Memory Disambiguation

Key feature from paper "R-HLS: An IR for Dynamic High-Level Synthesis...":
- Analyzes memory access patterns
- Determines when memory operations can execute in parallel
- Generates appropriate hardware interfaces (AXI, etc.)

## FIRRTL Generation

### FIRRTL Overview

FIRRTL (Flexible Intermediate Representation for RTL) is a hardware description language that enables:
- Modular hardware design
- Composite aggregation
- Memory hierarchies
- Wire bundles

### Key FIRRTL Constructs

#### Modules

- Input/output ports
- Statements (seq, par, when, etc.)
- Hardware registers and wires

#### Memory

- `mem` keyword for memory declarations
- Read/write interfaces
- Memory latency and timing

#### Control Flow

- `when` for conditional execution
- Hardware control signals
- State machine generation

### FIRRTL Output Example

```firrtl
circuit Example:
  module Example:
    input clock: Clock
    input reset: Reset
    output io: UInt<8>

    // Hardware registers
    reg counter: UInt<8>, clock with : (reset => (reset, 0))

    // Combinational logic
    wire next_val = counter +% 1

    // State update
    counter := next_val

    // Output
    io := counter
````

## AXI Interface Generation

### AXI Protocol Support

- **AXI4**: Standard Advanced eXtensible Interface
- **AXI4-Lite**: Simplified configuration interface
- **AXI4-Stream**: Streaming data interface

### Key Classes

- **`VerilatorHarnessAxi`**: AXI interface wrapper
  - Location: `jlm/hls/backend/verilator-harness-axi.cpp`
  - Purpose: Generate SystemVerilog testbench with AXI interfaces
- **`VerilatorHarnessHls`**: High-level Verilator interface
  - Purpose: Connect HLS-generated RTL with verification IP

## System Generation

### Hardware System Components

- **GPara**: Hardware parameter generation
- **Memory Controllers**: AXI memory interface
- **Clock/Reset**: Global clock and reset distribution
- **Testbenches**: Verilator-ready test environments

### JSON-based Configuration

- **JSON Output**: `jlm/hls/backend/json-hls.*`
- **Purpose**: Generate hardware configuration files
- **Features**: Customizable parameters, memory maps, interfaces

### Dot-based Visualization

- **Dot Output**: `jlm/hls/HlsDotWriter.*`
- **Purpose**: Visualize HLS design hierarchy
- **Usage**: `jhls --emit-dot input.c -o output.dot`

## Build Requirements
- **CIRCT** (install via `./scripts/build-circt.sh`)
- **MLIR 18** (required for CIRCT)
- **Verilator 4.038** (for simulation)

-------

### CIRCT Installation

```bash
./scripts/build-circt.sh
````

### Configure with HLS

```bash
./configure --enable-hls
make clean
make all
````

### Dependencies

- **CIRCT**: MLIR-based hardware IR framework
- **MLIR 18**: Middle-level IR infrastructure
- **Verilator 4.038**: Verilog simulation and linting

## Example Usage

### Simple Workflow

```bash
# 1. Compile C code
jlc -emit-llvm simple.c -o simple.bc

# 2. Generate FIRRTL
jhls --emit-firrtl simple.bc -o simple.fir

# 3. Convert to Verilog
jhls --emit-verilog simple.bc -o simple.v
````

### Advanced Workflow with Verilator

```bash
# 1. Compile with debug info
jlc -g -emit-llvm design.c -o design.bc

# 2. Generate Verilator harness
jhls --emit-verilator design.bc -o design.harness

# 3. Simulate with Verilator
verilator --cc design.v && make -C obj_dir -f Vdesign.mk

# 4. Run simulation
./obj_dir/Vdesign
````

## Key Outputs

### FIRRTL Output

- File extension: `.fir`
- Contains: Hardware description in FIRRTL
- Use: Input to FIRRTL tools or Verilog conversion

### Verilog Output

- File extension: `.v`
- Contains: SystemVerilog/RTL code
- Use: Synthesis, simulation, formal verification

### Testbench Output

- File extension: `.harness` or `.cpp`
- Contains: Verilator-ready testbench
- Use: Cycle-accurate simulation

## Optimization Options

### Area Optimization

```bash
jhls --area=aggressive input.c -o output.fir
````

### Latency Optimization

```bash
jhls --latency=minimum input.c -o output.fir
````

### Memory Optimization

```bash
jhls --memory=shared input.c -o output.fir
````

## Debugging and Visualization

### Debug Output

```bash
jhls -debug input.c -o output.fir
````

### Emit Intermediate Representations

```bash
# Emit R-HLS (if supported)
jhls --emit-rhls input.c -o output.rhls

# Emit FIRRTL
jhls --emit-firrtl input.c -o output.fir
````

### Generate Dot Visualization

```bash
jhls --emit-dot input.c -o output.dot
dot -Tpng output.dot -o output.png
````

## Related Research

Key papers describing HLS implementation:

1. **R-HLS Paper**: Metz, Reissmann, Själander
   - "R-HLS: An IR for Dynamic High-Level Synthesis and Memory Disambiguation based on Regions and State Edges"
   - ICCAD 2024

2. **RVSDG Paper**: Reissmann, Meyer, Bahmann, Själander
   - "RVSDG: An Intermediate Representation for Optimizing Compilers"
   - ACM TECS 2020

## Common Issues

### Missing CIRCT

Ensure CIRCT is built and configured with `--enable-hls`.

### Memory Access Errors

Check for unanalyzed memory dependencies in the input code.

### Complex Control Flow

Some control flow patterns may need restructuring for hardware.

## Quick Reference

| Command | Description |
|---------|-------------|
| `jhls --emit-firrtl` | Generate FIRRTL |
| `jhls --emit-verilog` | Generate Verilog |
| `jhls --emit-dot` | Generate visualization |
| `make check` | Run unit tests |

## The Full HLS Pipeline

### Overview

The HLS pipeline transforms C/C++ code into hardware descriptions through three main stages:

```
LLVM IR → RVSDG → R-HLS → FIRRTL → Verilog
```

### Stage 1: LLVM IR to RVSDG

**Location**: `tools/jlc/` + `jlm/llvm/frontend/`

- Clang/LLVM frontend compiles C to LLVM IR
- RVSDG converter transforms LLVM IR to Region-based Value-State Dependence Graph
- `LambdaNode` represents functions, `GammaNode` represents branches, `ThetaNode` represents loops

### Stage 2: RVSDG to R-HLS (jlm/hls/backend/rvsdg2rhls/)

**Key Transformations:**

| Transformation | File | Purpose |
|---|--|-|
| GammaConversion | GammaConversion.cpp | Convert gamma nodes to MuxOperation |
| ThetaConversion | ThetaConversion.cpp | Convert theta nodes to loop structures |
| MemoryConverter | mem-conv.cpp | Convert memory operations to explicit ports |
| UnusedStateRemoval | UnusedStateRemoval.cpp | Remove unused state edges |

**BundleType Creation:**
```cpp
// In mem-conv.cpp, ConvertMemory()
auto responseType = get_mem_res_type(BitType::Create(portWidth));   // BundleType for responses
auto requestType = get_mem_req_type(BitType::Create(portWidth), hasWrite);  // BundleType for requests
newArgumentTypes.push_back(responseType);  // Memory responses go IN (from hardware)
newResultTypes.push_back(requestType);     // Memory requests go OUT (to hardware)
```

### Stage 3: R-HLS to FIRRTL (jlm/hls/backend/rhls2firrtl/)

**Key Classes:**

| Class | File | Purpose |
|---|--|-|
| RhlsToFirrtlConverter | RhlsToFirrtlConverter.cpp | Main conversion orchestrator |
| VerilatorHarnessAxi | VerilatorHarnessAxi.cpp | Generate AXI testbench |
| VerilatorHarnessHls | verilator-harness-hls.cpp | Generate Verilator harness |

**BundleType Fields:**

| Field | Type | Purpose |
|---|--|-|
| addr | Pointer | Memory address |
| data | BitType | Data value |
| id | BitType<8> | Transaction ID |
| size | BitType<4> | Transfer size |
| write | BitType<1> | Write enable |

### Stage 4: FIRRTL to Verilog

Uses CIRCT's FIRRTL compiler to lower FIRRTL to SystemVerilog.

### BundleType Understanding

**Why BundleType Exists:**
- Hardware memory operations need multiple signals (addr, data, control)
- BundleType groups these into a single hardware interface
- Type filtering separates memory ports from regular data

**Type Filtering:**

| Function | Returns | Filters By |
|---|--|--|
| get_reg_args() | Non-BundleType arguments | `!rvsdg::is<BundleType>()` |
| get_reg_results() | Non-BundleType results | `!rvsdg::is<BundleType>()` |
| get_mem_reqs() | BundleType results | `rvsdg::is<BundleType>()` |
| get_mem_resps() | BundleType arguments | `rvsdg::is<BundleType>()` |

## HLS Backend Unit Testing

### Overview

The HLS backend uses Google Test (gtest) for unit testing. Testing HLS-specific code requires understanding:
- Protected method access patterns
- BundleType type filtering
- LambdaNode finalization and subregion results
- Abstract base class instantiation for testing

### Test Patterns

#### Testing Protected Methods

HLS backend classes often have protected methods that need testing. Use a test helper class:

```cpp
#include <jlm/hls/backend/rhls2firrtl/base-hls.hpp>

class TestableBaseHLS : public jlm::hls::BaseHLS
{
public:
  using jlm::hls::BaseHLS::get_port_name;
  using jlm::hls::BaseHLS::get_node_name;
  using jlm::hls::BaseHLS::get_reg_args;
  using jlm::hls::BaseHLS::get_reg_results;
  using jlm::hls::BaseHLS::get_mem_reqs;
  using jlm::hls::BaseHLS::get_mem_resps;
  using jlm::hls::BaseHLS::JlmSize;
  using jlm::hls::BaseHLS::node_map;
  using jlm::hls::BaseHLS::output_map;

  // Override pure virtual methods
  std::string GetText(llvm::LlvmRvsdgModule & rm) override { return ""; }
  std::string extension() override { return ".txt"; }
};

TEST(HlsTests, TestProtectedMethod)
{
  auto portName = TestableBaseHLS().get_port_name(input);
  EXPECT_EQ(portName, "a0");
}
```

#### Testing BundleType Filtering

The HLS utility functions filter by type:

| Function | Returns | Filters By |
|----------|---------|------------|
| `get_reg_args()` | Non-BundleType arguments | `!rvsdg::is<BundleType>()` |
| `get_reg_results()` | Non-BundleType results | `!rvsdg::is<BundleType>()` |
| `get_mem_reqs()` | BundleType results | `rvsdg::is<BundleType>()` |
| `get_mem_resps()` | BundleType arguments | `rvsdg::is<BundleType>()` |

```cpp
// Memory request BundleType
std::vector<std::pair<std::string, std::shared_ptr<const jlm::rvsdg::Type>>> elements;
elements.emplace_back("addr", jlm::llvm::PointerType::Create());
elements.emplace_back("data", bitType);
auto memReqType = std::make_shared<jlm::hls::BundleType>(std::move(elements));

// Register arguments exclude BundleType
auto regArgs = TestableBaseHLS().get_reg_args(*lambda);
EXPECT_EQ(regArgs.size(), 1);  // Only non-bundle arguments

// Memory responses include only BundleType
auto memResps = TestableBaseHLS().get_mem_resps(*lambda);
EXPECT_EQ(memResps.size(), 1);  // Only bundle arguments
```

#### LambdaNode Finalization

LambdaNode requires `finalize()` to populate subregion results:

```cpp
// Create lambda with function type
auto lambda = LambdaNode::Create(
    region,
    LlvmLambdaOperation::Create(functionType, "f", Linkage::externalLinkage));

// Finalize to create results in subregion
auto f = lambda->finalize({ returnValues });

// Now subregion()->Results() is populated
EXPECT_EQ(lambda->subregion()->nresults(), returnValues.size());
```

**Key Differences:**
- `GetFunctionArguments()` - Returns function type arguments (always populated)
- `subregion()->Arguments()` - Returns actual region arguments (populated by `finalize()`)
- `GetFunctionResults()` - Returns function type results (always populated)
- `subregion()->Results()` - Returns actual region results (populated by `finalize()`)

#### Accessing Results by Index

```cpp
// Correct: use result() method
auto result = lambda->subregion()->result(index);

// Incorrect: Results() returns IteratorRange, not vector
// auto result = lambda->subregion()->Results()[index];  // Won't compile
```

### Build System

#### Adding HLS Tests

Add test sources to `jlm/hls/Makefile.sub`:

```makefile
run-libhls-tests_SOURCES += \
    jlm/hls/backend/rhls2firrtl/BaseHlsTests.cpp

run-libhls-tests_LIBS = libhls libllvm librvsdg libutil
$(eval $(call common_test,run-libhls-tests))
```

#### Running HLS Tests

```bash
# Run all HLS tests
make check

# Run specific test binary
./build/run-libhls-tests

# Run specific test suite
./build/run-libhls-tests --gtest_filter=BaseHlsTests.*
```

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| `JlmSize() throws` | TestType not supported | Use `BitType::Create(n)` instead |
| `Expected bundle, got TestType` | Wrong type in test setup | Use `BundleType` for memory types |
| `subregion()->Results() empty` | Forgot to call `finalize()` | Call `lambda->finalize()` before testing |
| `Abstract class instantiation` | Pure virtual `GetText()` not overridden | Override `GetText()` and `extension()` |

### JlmSize() Limitations

```cpp
// Supported types
JlmSize(BitType::Create(32));     // Returns 32
JlmSize(PointerType::Create());   // Returns 64 (on 64-bit)

// BundleType returns 0 (hack for get_node_name)
JlmSize(bundleType);              // Returns 0

// TestType not supported - throws exception
JlmSize(TestType::createValueType());  // Error!
```

### Related Files

| File | Purpose |
|------|---------|
| `jlm/hls/backend/rhls2firrtl/base-hls.hpp` | BaseHLS class with utility functions |
| `jlm/hls/ir/hls.hpp` | BundleType and HLS types |
| `jlm/rvsdg/region.hpp` | Region interface for subregion access |
| `jlm/rvsdg/lambda.hpp` | LambdaNode and finalization |

## SEE ALSO

### Code Generation Guidelines

- All generated C++ files must be formatted with `make format` (clang‑format) and pass `make tidy` (clang‑tidy).
- Follow the **File Header Template** described in each skill for consistency.
- Refer to the `coding-style` SKILL for detailed formatting rules.


- **MAIN SKILL**: [Project overview](skill:main)
- **LLVM SKILL**: [LLVM mode details](skill:llvm)
- **MLIR SKILL**: [MLIR mode details](skill:mlir)
- **CIRCT SKILL**: [CIRCT integration](skill:circt)
- **TESTING SKILL**: [Testing framework](skill:testing)

## RVSDG to R-HLS Conversion Tests

### Overview

Tests for the RVSDG to R-HLS conversion pipeline verify that HLS transformations correctly process RVSDG graphs. These tests use Google Test and follow JLM's testing conventions.

### StreamConversion Tests

The `StreamConversion` pass converts HLS stream operations (producer/consumer pairs) into buffer-based communication. Test cases should verify:
- Lambda structure preservation
- Constant value handling
- No-op behavior for lambdas without stream operations

#### Example Pattern

```cpp
TEST(StreamConversionTests, TestSimpleStream)
{
  using namespace jlm::llvm;

  auto bit32Type = jlm::rvsdg::BitType::Create(32);

  LlvmRvsdgModule rm(jlm::util::FilePath("", "", ""));
  auto & rvsdg = rm.Rvsdg();

  auto lambda = jlm::rvsdg::LambdaNode::Create(
      rvsdg.GetRootRegion(),
      LlvmLambdaOperation::Create(
          jlm::rvsdg::FunctionType::Create({ bit32Type }, { bit32Type }),
          "f",
          Linkage::externalLinkage));

  auto * constant = &jlm::rvsdg::BitConstantOperation::create(*lambda->subregion(), { 32, 42 });

  auto lambdaOutput = lambda->finalize({ constant });
  jlm::rvsdg::GraphExport::Create(*lambdaOutput, "f");

  // Act
  jlm::util::StatisticsCollector statisticsCollector;
  jlm::hls::StreamConversion::CreateAndRun(rm, statisticsCollector);

  // Assert
  EXPECT_EQ(rvsdg.GetRootRegion().numNodes(), 1);
}
```

#### Important Notes

- Use `{ numBits, value }` format for `BitConstantOperation::create()`
- The function returns a reference; use `&` to get pointer if needed
- Don't call `jlm::rvsdg::view()` in tests - output goes to stdout and clutters test results

### Common Issues with Memory State Handling

| Issue | Cause | Solution |
|-------|-------|----------|
| `GetMemoryStateRegionArgument throws assertion` | Memory state argument doesn't exist | Use `tryGetMemoryStateEntrySplit()` which returns nullptr when no memory state exists |

The pass implementation (`decouple-mem-state.cpp`) has a bug where it calls `GetMemoryStateRegionArgument(*lambda)` before checking for null. This function throws an assertion if there's no memory state, rather than returning null. The workaround is to ensure tests always include memory state arguments.

### Related Files

| File | Purpose |
|------|---------|
| `jlm/hls/backend/rvsdg2rhls/stream-conv.cpp` | StreamConversion transformation |
| `jlm/hls/backend/rvsdg2rhls/decouple-mem-state.cpp` | MemoryStateDecoupling transformation |
| `jlm/hls/backend/rvsdg2rhls/GammaConversion.cpp` | Gamma node to Mux conversion |
| `jlm/hls/backend/rvsdg2rhls/ThetaConversion.cpp` | Theta node to loop conversion |

## SEE ALSO

- **MAIN SKILL**: [Project overview](skill:main)
- **LLVM SKILL**: [LLVM mode details](skill:llvm)
- **MLIR SKILL**: [MLIR mode details](skill:mlir)
- **CIRCT SKILL**: [CIRCT integration](skill:circt)
- **TESTING SKILL**: [Testing framework](skill:testing)
