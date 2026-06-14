---
name: circt
description: JLM compiler CIRCT integration. Use when working with CIRCT FIRRTL lowering, Verilog generation, or hardware-specific MLIR dialects.
---

## Quick Checklist
- Build CIRCT: `./scripts/build-circt.sh`
- Enable HLS in configure: `./configure --enable-hls`
- Verify FIRRTL dialect loading with `firtool --help`

-------

## Overview

CIRCT (CIRCT is a Compiler Infrastructure for Robotics and Control Technologies) provides the FIRRTL and Verilog lowering capabilities for JLM's High-Level Synthesis mode. CIRCT is built on top of MLIR and provides hardware-specific dialects and transformations.

**Note**: This file documents the CIRCT integration in the JLM project. CIRCT is only used when HLS is enabled.

## CIRCT in JLM

### Role in HLS Pipeline

```
LLVM IR → RVSDG → R-HLS → FIRRTL (via CIRCT) → Verilog (via CIRCT)
````

### Integration Points

- **FIRRTL Dialect**: Used for hardware description
- **Verilog Lowering**: Converts FIRRTL to SystemVerilog
- **Hardware-Specific Passes**: Optimization and transformation passes
- **AXI Protocol Support**: Memory-mapped bus interface generation

## CIRCT Location

- **Repository**: `/build-circt/circt.git/`
- **Build Script**: `./scripts/build-circt.sh`
- **Integration**: Automatically linked when building JLM with `--enable-hls`

## CIRCT Dialects Used

### FIRRTL Dialect

- **Purpose**: Hardware description language
- **Key Features**:
  - Composite aggregation
  - Memory hierarchies
  - Modular design
  - Configurable parameters
- **File Extensions**: `.fir`

### Hardware-Specific Dialects

- **AXI Dialect**: AXI bus protocol support
- **Memory Dialect**: Memory system modeling
- **Clock Dialect**: Clock domain management
- **Reset Dialect**: Reset signal handling

### Standard MLIR Dialects

- **Arith**: Arithmetic operations
- **SCF**: Structured control flow
- **Func**: Function definitions
- **LLVM**: Low-level hardware operations

## FIRRTL Lowering Pipeline

### Verilog Generation

The FIRRTL-to-Verilog conversion involves:

1. **FIRRTL Input**: Module declarations with FIRRTL syntax
2. **Lowering Passes**:
   - `LowerMemories`: Convert FIRRTL memories to Verilog
   - `LowerVerif`: Lower verification constructs
   - `LowerAnnotations`: Process annotations
3. **Verification**: Verilog code inspection
4. **Output**: SystemVerilog/RTL code

### AXI Interface Generation

- **Memories**: Convert to AXI-ready memory systems
- **Interfaces**: Generate AXI4/AXI4-Lite interfaces
- **Wrappers**: Create Verilator-compatible wrappers
- **Testbenches**: Generate AXI-specific testbench code
- **Memory Hierarchies**: Multi-level memory system generation

## Key CIRCT Files for HLS

### JLM HLS Backend Files

- **Location**: `jlm/hls/backend/firrtl2verilog/`
- **Purpose**: JLM-specific CIRCT glue code

### FIRRTL2Verilog Converter

- **File**: `FirrtlToVerilogConverter.cpp` (conceptual)
- **Features**:
  - Module-level conversion
  - Port mapping
  - Memory serialization
  - Clock/reset handling

### AXI Harness Files

- **VerilatorHarnessAxi**: AXI4 interface wrapper
- **VerilatorHarnessHls**: High-level harness generation
- **Purpose**: Simulate HLS-generated RTL with AXI bus
- **Memory System Files**: Configuration and interface generation

## AXI Protocols Supported

### AXI4 Protocol

- **5 Channel Architecture**: AR, AW, R, W, B
- **Independent Address/Data**: Separate channels
- **Burst Transfers**: Burst operations
- **Out-of-order Completion**: Non-blocking transactions

### AXI4-Lite Protocol

- **Simplified**: Single address/data channel
- **Configuration Interface**: For register access
- **Single Outstanding Transaction**: No speculation

### AXI4-Stream Protocol

- **Streaming Data**: FIFO-based data flow
- **Simple Interfacing**: Source/sink paradigm
- **Low Latency**: Single-cycle transfers

## FIRRTL to Verilog Conversion

### Module Conversion

- **Input Ports**: Convert to Verilog `input`
- **Output Ports**: Convert to Verilog `output`
- **Reg vs Wire**: Register inference
- **Combinational Logic**: Always @(*) blocks

### Memory Conversion

- **FIRRTL `mem`**: Convert to Verilog arrays
- **Read Ports**: Convert to async/read-first variables
- **Write Ports**: Convert to synchronous writes

### Control Constructs

- **FIRRTL `when`**: Convert to conditional logic
- **FIRRTL `seq/par`**: Convert to sequential blocks
- **FIRRTL `printf`**: Convert to SystemVerilog $display

## Verilator Integration

### Verilation Process

1. **FIRRTL Generation**: `jhls --emit-firrtl`
2. **FIRRTL to Verilog**: `jhls --emit-verilog`
3. **Verilator Compilation**: `verilator --cc design.v`
4. **Makefile Generation**: Automatic testbench creation
5. **Simulation**: Cycle-accurate RTL simulation

### Testbench Generation

- **VerilatorHarnessAxi**: Generates AXI testbench
- **Clock/Reset**: Automatic clock generation
- **Stimulus Generation**: Random or patterned stimuli
- **Monitors**: Assertion and coverage collection

## Build and Installation

### Build CIRCT

```bash
# Build CIRCT from source
./scripts/build-circt.sh

# Or download pre-built
wget https://github.com/llvm/circt/releases/download/...
````

### Configure JLM with CIRCT

```bash
./configure --enable-hls
make clean
make all
````

### Verilator Installation

```bash
# Install Verilator 4.038
sudo apt-get install verilator=4.038
# or build from source
git clone https://git.veripool.org/~mition/verilator
cd verilator
git checkout v4.038
autoconf
./configure && make
sudo make install
````

## Usage Examples
```bash
# Compile C to LLVM IR
jlc -emit-llvm design.c -o design.bc

# Generate FIRRTL via CIRCT
jhls --emit-firrtl design.bc -o design.fir

# Convert FIRRTL to Verilog
jhls --emit-verilog design.bc -o design.v
```


### Simple FIRRTL Generation

```bash
# 1. Compile C code
jlc -emit-llvm design.c -o design.bc

# 2. Generate FIRRTL (using CIRCT)
jhls --emit-firrtl design.bc -o design.fir
````

### FIRRTL to Verilog

```bash
# 1. Generate Verilog
jhls --emit-verilog design.bc -o design.v

# 2. Verify syntax
verilator --lint-only design.v
````

### Simulation with Verilator

```bash
# 1. Generate harness
jhls --emit-verilator design.bc -o design_harness

# 2. Build and run
verilator --cc design.v && make -C obj_dir -f Vdesign.mk
./obj_dir/Vdesign +verilator+rand+reset+1
````

## Key CIRCT Tools

### FIRRTL Tools

- **firtool**: FIRRTL compiler and optimizer
- **verifglen**: Verilog generation
- **firrtl_to_verilog**: Direct FIRRTL-to-Verilog

### CIRCT Dialect Tools

- **mlir-opt**: MLIR-based dialect operations
- **firrtl-convert**: Dialect conversion utilities
- **cstoconvert**: Custom conversion tools
- **memory-convert**: Memory system conversion utilities

## Debugging CIRCT Issues

### Missing CIRCT

Ensure CIRCT is built and `--enable-hls` is used in configure.

### Verilog Generation Errors

Check FIRRTL syntax and supported FIRRTL features.

### AXI Interface Issues

Verify memory configuration and AXI protocol settings.

### Memory System Debugging

- Check memory hierarchy configuration files
- Verify AXI protocol compatibility
- Inspect multi-bank memory arbitration
- Validate burst transfer configurations

## SEE ALSO

- **MAIN SKILL**: [Project overview](skill:main)
- **LLVM SKILL**: [LLVM mode details](skill:llvm)
- **HLS SKILL**: [HLS mode details](skill:hls)
- **MLIR SKILL**: [MLIR mode details](skill:mlir)
- **TESTING SKILL**: [Testing framework](skill:testing)
