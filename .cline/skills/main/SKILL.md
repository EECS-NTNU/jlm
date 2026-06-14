---
name: main
description: JLM compiler overview and main functionality. Use for general questions about the JLM project, its core components, and RVSDG-based compiler architecture.
---

# JLM Compiler - Main Overview

Jlm is a research compiler/optimizer that uses the Regionalized Value State Dependence Graph (RVSDG) as its intermediate representation. It's designed for compiler research, optimization, and high-level synthesis.

## Key Features
- Experimental compiler framework
- RVSDG-based intermediate representation
- Support for multiple frontends/backends
- Extensive optimization support
- High‑level synthesis capabilities

## Getting Started Checklist
- **Configure & build**: `./configure.sh && make all`
- **Compile a simple program**:
  ```bash
  jlc -emit-llvm hello.c -o hello.bc
  jlm-opt hello.bc -o hello.opt.bc
  ```
- **Generate hardware (HLS)**:
  ```bash
  jhls --emit-firrtl hello.bc -o hello.fir
  ```

## Core Concept: RVSDG

The Regionalized Value State Dependence Graph (RVSDG) is the core IR with the following key characteristics:

- **Value dependences**: Data‑flow relationships between computations
- **State dependences**: Control‑flow and memory state tracking
- **Regions**: Hierarchical structure with scoped variables
- **Perfect reconstructability**: Control flow can be exactly reconstructed from RVSDG

See: `jlm/rvsdg/` directory for RVSDG implementation

**SEE ALSO**: [LLVM SKILL](skill:llvm) for LLVM frontend conversion details, [HLS SKILL](skill:hls) for R‑HLS extensions, [MLIR SKILL](skill:mlir) for MLIR dialect mappings

## Three Main Functionalities

### LLVM Mode
Reads LLVM IR, converts to RVSDG, applies optimizations, and converts back to LLVM IR.

**Tools**: `jlc` (C compiler), `jlm-opt` (optimizer)

**See**: [LLVM SKILL](skill:llvm) for detailed information

### HLS Mode
Reads LLVM IR, converts to RVSDG, performs HLS‑specific transformations, generates FIRRTL hardware description, and can emit Verilog.

**Tool**: `jhls` (HLS frontend)

**See**: [HLS SKILL](skill:hls) for detailed information

### MLIR Mode
Converts JLM‑native RVSDG to/from MLIR RVSDG dialect, enabling interoperability with the MLIR ecosystem.

**See**: [MLIR SKILL](skill:mlir) for detailed information

## Build Configuration

### Basic Build
```bash
./configure.sh
make all
```

### HLS Build (requires CIRCT)
```bash
./scripts/build-circt.sh
./configure --enable-hls
make clean  # recommended after changing build config
make all
```

### MLIR Build (requires MLIR RVSDG dialect)
```bash
./scripts/build-mlir.sh
./configure --enable-mlir
make clean  # recommended after changing build config
make all
```

### Debug Build
```bash
./configure --target debug
make all
```

### Coverage Build
```bash
./configure --enable-coverage
make all
make coverage  # generates coverage report
```

## Testing

### Unit Tests
```bash
make check
```

### Valgrind Testing
```bash
make valgrind-check
```

### Header Check
```bash
make check-headers
```

## Documentation

### Generate Doxygen
```bash
make docs
```
View at: `docs/html/index.html`

## Key Directories
- `jlm/rvsdg/`: Core RVSDG implementation
- `jlm/llvm/`: LLVM frontend/backend
- `jlm/hls/`: High‑level synthesis backend
- `jlm/mlir/`: MLIR frontend/backend
- `tests/`: Unit tests and integration tests
- `tools/`: Main command‑line tools

## Related Publications
Key research papers:

1. **RVSDG Introduction** – N. Reissmann et al., *RVSDG: An Intermediate Representation for Optimizing Compilers*, ACM TECS 2020.
2. **R‑HLS (Dynamic HLS)** – D. Metz, N. Reissmann, M. Själander, *ICCAD 2024*.
3. **PIP Analysis** – H.R. Krogstie et al., *CGO 2026*.
4. **Control Flow Reconstruction** – H. Bahmann et al., *ACM TACO 2015*.

## Memory Analysis
- **Points‑to Analysis (PIP)** – available in LLVM mode.
- **Memory Disambiguation** – core feature in HLS mode (see HLS SKILL).
- **Delta Operators** – trace memory state changes in RVSDG.
- **Limitations** – interprocedural analysis limited by function inlining decisions.

## Optimization Pipeline
The optimizer (`jlm‑opt`) applies optimizations in stages:

- **Default Level**: basic passes (constant propagation, dead‑code elimination, simple inlining).
- **Customization**: `--passes` flag to specify custom sequences.
- **Available Passes**: `mem2reg`, `inlining`, `dead-code-elim`, `control-flow-simplify`, etc.

```bash
# Default optimization
jlm-opt input.bc -o output.bc

# Custom passes
jlm-opt --passes="mem2reg,inlining" input.bc -o output.bc

# Debug output
jlm-opt -debug input.bc -o output.bc
```

## Error Handling and Diagnostics
- **Debug Output**: `-debug`
- **Verbose Mode**: `-v`
- **Statistics**: `--stats`
- **Visualization**: `--emit-dot` (see LLVM SKILL)

### Exit Codes
1 – general errors, 2 – configuration/dependency errors, 3 – conversion failures, 4 – optimization errors.

## Architecture Overview
```
           C Code
               ↓
      [jlc] LLVM IR
               ↓
            [RVSDG]
    /     |     \
LLVM‑IR  R‑HLS   MLIR RVSDG
|        |       |
|        ↓       |
|    [FIRRTL]    |
|        ↓       |
|    [Verilog]   |
 ↓               ↓
[Optimized Code] [Hardware]
```

## Tool Overview
- **jlc** – C compiler (LLVM frontend)
- **jlm‑opt** – RVSDG optimizer
- **jhls** – High‑level synthesis tool

### MLIR Conversion Tools
- `JlmToMlirConverter` – RVSDG → MLIR RVSDG dialect
- `MlirToJlmConverter` – MLIR → RVSDG

## Dependencies
- **LLVM Mode**: Clang/LLVM 18, Doxygen 1.9.8, `lit` 18.
- **HLS Mode**: MLIR 18, CIRCT (see CIRCT SKILL), Verilator 4.038.
- **MLIR Mode**: MLIR 18, custom MLIR RVSDG dialect.

## Quick Start
```bash
# Compile & optimize
jlc -emit-llvm prog.c -o prog.bc
jlm-opt prog.bc -o prog.opt.bc

# Generate hardware (HLS)
jhls --emit-firrtl prog.bc -o prog.fir
```

## SEE ALSO
- **MAIN SKILL**: [Project overview](skill:main)
- **LLVM SKILL**: [LLVM mode details](skill:llvm)
- **HLS SKILL**: [High‑Level Synthesis mode](skill:hls)
- **MLIR SKILL**: [MLIR mode details](skill:mlir)
- **CIRCT SKILL**: [CIRCT integration](skill:circt)
- **TESTING SKILL**: [Testing framework](skill:testing)