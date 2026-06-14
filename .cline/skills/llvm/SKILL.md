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
name: llvm
description: JLM compiler LLVM mode. Use when working with LLVM IR to RVSDG conversion, optimization passes, or when you need to understand how the LLVM frontend and backend work in the JLM compiler.
---

# JLM Compiler - LLVM Mode

The LLVM mode provides conversion between LLVM IR and RVSDG, allowing optimizations to be applied using JLM's RVSDG pass infrastructure. This mode is the foundation for both the C compiler (`jlc`) and the optimizer (`jlm-opt`).

## Tools

### `jlc` - JLM C Compiler

- **Purpose**: Compile C code to LLVM bitcode
- **Usage**: `jlc [options] input.c -o output.bc`
- **Process**: C → LLVM IR → RVSDG → Optimized RVSDG → LLVM IR → Bitcode
- **Location**: `tools/jlc/jlc.cpp`

### `jlm-opt` - JLM Optimizer

- **Purpose**: Apply RVSDG optimizations to LLVM bitcode
- **Usage**: `jlm-opt [options] input.bc -o output.bc`
- **Process**: LLVM IR → RVSDG → Apply Optimizations → Emit LLVM IR
- **Location**: `tools/jlm-opt/jlm-opt.cpp`

## Frontend: LLVM IR to RVSDG

### Main Conversion Entry Point

- **Class**: `LlvmModuleConversion` in `jlm/llvm/frontend/LlvmModuleConversion.hpp`
- **Function**: `ConvertLlvmModule(::llvm::Module & module)`
- **Process**:
  1. Parse LLVM module
  2. Create RVSDG graph structure
  3. Convert functions to RVSDG
  4. Build interprocedural graph

### Key Classes

- **LlvmModuleConversion**: Main conversion orchestrator
- **InterProceduralGraphModule**: Top-level RVSDG container for LLVM code
- **TypeConverter**: Converts LLVM types to RVSDG types

### Conversion Components

#### Control Flow Conversion

- **File**: `jlm/llvm/frontend/ControlFlowRestructuring.hpp`
- **Purpose**: Convert LLVM CFG to RVSDG regions
- **Key Operations**:
  - Basic block analysis
  - SSA construction
  - Region boundary detection
  - Omega node creation for control flow

#### Function Call Conversion

- **File**: `jlm/llvm/frontend/FunctionCallTests.cpp` (test examples)
- **Features**:
  - Direct calls
  - Indirect calls
  - Variadic functions
  - External function handling

#### Memory Operations

- **Load Conversion**: `jlm/llvm/frontend/LoadTests.cpp`
- **Store Conversion**: `jlm/llvm/frontend/StoreTests.cpp`
- **Features**:
  - Pointer analysis
  - Memory state tracking
  - Alignment handling
  - Volatile memory access

#### Specialized Operations

- **FNeg**: Floating-point negation (`jlm/llvm/frontend/FNegTests.cpp`)
- **Select**: Conditional selection operations (`jlm/llvm/frontend/SelectTests.cpp`)
- **Casts**: Type conversions (`jlm/llvm/frontend/CastingTests.cpp`)
- **Arithmetic Intrinsics**: Specialized math operations

#### Mixed Control/Data Flow

- **Three-Address Code**: `jlm/llvm/frontend/ThreeAddressCodeConversionTests.cpp`
- **LlvmPhi**: SSA phi node conversion
- **Endless Loops**: Special handling for infinite loops

#### Interprocedural Analysis

- **File**: `jlm/llvm/frontend/InterProceduralGraphConversion.hpp`
- **Features**:
  - Call graph construction
  - Function inlining decisions
  - External function modeling
  - Modability analysis

## Optimizations
- **Basic**: Constant propagation, dead‑code elimination.
- **Medium**: Loop optimizations, inlining.
- **High**: Parallelization, scheduling.

See `jlm/llvm/opt/` for full list.

-------

Optimizations are applied after conversion to RVSDG. See `jlm/llvm/opt/` directory for optimization passes.

### Optimization Workflow

1. **Basic Optimizations**: Constant propagation, dead code elimination
2. **Medium-level**: Loop optimizations, inlining
3. **High-level**: Pointer analysis, memory optimizations
4. **Advanced**: Parallelization, scheduling

### Key Optimization Files

- `ControlFlowRestructuring.cpp`: CFG manipulation
- Various test files show optimization scenarios

## Backend: RVSDG to LLVM IR

### Conversion Pipeline

#### RVSDG to InterProceduralGraph

- **Class**: `RvsdgToIpGraphConverter` (`jlm/llvm/backend/RvsdgToIpGraphConverter.hpp`)
- **Purpose**: Convert RVSDG to interprocedural graph structure
- **Features**:
  - Function extraction
  - Call graph reconstruction
  - Memory state recovery
  - Type mapping

#### InterProceduralGraph to LLVM IR

- **Class**: `IpGraphToLlvmConverter` (`jlm/llvm/backend/IpGraphToLlvmConverter.hpp`)
- **Purpose**: Convert to LLVM IR
- **Features**:
  - Module creation
  - Function emission
  - Instruction generation
  - Metadata preservation

### Key Conversion Features

- **Control Flow**: Regions → LLVM basic blocks
- **Memory Operations**: Delta operators → load/store instructions
- **Operators**:
  - Binary: `+`, `-`, `*`, `/`, `%`, bitwise ops
  - Unary: Negation, bitwise not
  - Comparisons: `==`, `<`, etc.
  - Float: Floating-point operations
  - Casts: Type conversions
- **Special Operations**: GEP, select, phi nodes, etc.

## Type System

### LLVM Type Conversion

- **File**: `jlm/llvm/ir/TypeConverter.h` (conceptual)
- **Supported Types**:
  - Integer: `i8`, `i16`, `i32`, `i64`, etc.
  - Floating-point: `float`, `double`
  - Pointer: `i8*`, function pointers
  - Vector: `<4 x i32>`, etc.
  - Aggregate: Structs, arrays
  - Opaque: Unnamed types

### RVSDG Type Mapping

- LLVM integers → RVSDG bitstrings
- LLVM floats → RVSDG fixed-point or float types
- LLVM pointers → RVSDG bitstrings (addresses)
- Aggregates → RVSDG struct types
- Arrays → RVSDG array types

## Example Workflow

### Compile C to Bitcode

```bash
jlc -emit-llvm example.c -o example.bc
```

### Optimize

```bash
jlm-opt example.bc -o example.opt.bc
```

### Optimization Pipeline

```bash
# Default optimization
jlm-opt input.bc -o output.bc

# Specific optimization passes
jlm-opt --passes="mem2reg,inlining" input.bc -o output.bc

# Debug output
jlm-opt -debug input.bc -o output.bc
```

## Key Files and Directories

- `jlm/llvm/frontend/`: LLVM IR to RVSDG conversion
- `jlm/llvm/backend/`: RVSDG to LLVM IR conversion
- `jlm/llvm/ir/`: Data structures and types
- `jlm/llvm/opt/`: Optimization passes
- `tools/jlc/`: C compiler
- `tools/jlm-opt/`: Optimizer

## Testing

### Unit Tests

```bash
make check
```

### Specific Test Files

- `LlvmModuleConversionTests.cpp`: Module-level tests
- `FunctionCallTests.cpp`: Function call handling
- `LoadTests.cpp`: Memory load operations
- `StoreTests.cpp`: Memory store operations

## Visualization

### RVSDG Dot Output

```bash
jlm-opt --emit-dot input.bc -o output.dot
```

### View with Graphviz

```bash
dot -Tpng output.dot -o output.png
````

View the RVSDG structure graphically.

## Debugging

### Debug Output

```bash
jlm-opt -debug input.bc -o output.bc
```

### Verbose Output

```bash
jlm-opt -v input.bc -o output.bc
```

### Statistics

```bash
jlm-opt --stats input.bc -o output.bc
```

## Common Issues

### Missing Passes

Ensure all required LLVM passes are available.

### Type Conversion Errors

Check for unsupported LLVM types in input.

### Control Flow Problems

Complex CFG structures may need simplification.

## Quick Reference
| Command | Description |
|---------|-------------|
| `jlc -emit-llvm src.c -o src.bc` | Compile C to LLVM bitcode |
| `jlm-opt src.bc -o out.bc` | Apply default optimizations |
| `jlm-opt --passes="mem2reg,inlining" src.bc -o out.bc` | Run specific passes |

-------

| Command | Description |
|---------|-------------|
| `jlc -emit-llvm` | Compile C to LLVM bitcode |
| `jlm-opt input.bc` | Optimize bitcode |
| `jlm-opt --emit-dot` | Generate RVSDG visualization |
| `make check` | Run unit tests |

## SEE ALSO

### Code Generation Guidelines

- All generated C++ files must be formatted with `make format` (clang‑format) and pass `make tidy` (clang‑tidy).
- Follow the **File Header Template** described in each skill for consistency.
- Refer to the `coding-style` SKILL for detailed formatting rules.


- **MAIN SKILL**: [Project overview](skill:main)
- **HLS SKILL**: [HLS mode details](skill:hls)
- **MLIR SKILL**: [MLIR mode details](skill:mlir)
- **CIRCT SKILL**: [CIRCT integration](skill:circt)
- **TESTING SKILL**: [Testing framework](skill:testing)
