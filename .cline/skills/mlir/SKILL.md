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
name: mlir
description: JLM compiler MLIR mode. Use when working with MLIR RVSDG dialect conversion, interoperability with MLIR ecosystem, or MLIR-related tooling.
---

# JLM Compiler - MLIR Mode

The MLIR mode provides conversion between JLM's native RVSDG and the MLIR RVSDG dialect. This enables interoperability with the MLIR ecosystem and allows using MLIR tools with JLM's RVSDG‑based compiler infrastructure.

## Architecture
### Conversion Pipeline
```
MLIR RVSDG Dialect ↔ JLM RVSDG ↔ MLIR RVSDG Dialect
``` 

The MLIR mode supports bidirectional conversion:
1. **JLM → MLIR**: Convert JLM RVSDG to MLIR RVSDG dialect  
2. **MLIR → JLM**: Convert MLIR RVSDG dialect back to JLM RVSDG

## Tools and Testing
- **Round‑trip testing**: `./scripts/jlc-mlir-roundtrip.py testfile.jlm`
- **Integer ops testing**: `./scripts/jlc-check-opt.py testfile.jlm --mlir`

## Frontend: MLIR → JLM Conversion
### Main Class
- **Class**: `MlirToJlmConverter`  
- **Location**: `jlm/mlir/frontend/MlirToJlmConverter.hpp`
- **Key methods**:
  - `ReadAndConvertMlir()`
  - `ConvertMlir()`
  - `ConvertRegion()`
  - `ConvertBlock()`
  - `ConvertOperation()`

### Supported Dialects
- RVSDG, JLM, Arith, LLVM

## Backend: JLM → MLIR Conversion
### Main Class
- **Class**: `JlmToMlirConverter`  
- **Location**: `jlm/mlir/backend/JlmToMlirConverter.hpp`
- **Key methods**:
  - `ConvertRvsdg()`
  - `ConvertRegion()`
  - `ConvertNode()`
  - `ConvertOperation()`

### Emission
- Standard MLIR text format with explicit dialect imports.

## Common Issues
- **Missing dialect**: Ensure `mlir_rvsdg` is built and loaded (`mlir-opt -load libmlirRvsdg.so`).  
- **Type mismatches**: Verify bit‑width consistency between MLIR and JLM types.  

## Quick Reference
| Command | Description |
|---------|-------------|
| `./scripts/jlc-mlir-roundtrip.py src.jlm` | Verify round‑trip conversion |
| `mlir-opt -pass-pipeline="..." file.mlir`   | Run custom MLIR passes |
| `firtool --emit-verilog file.fir`          | Convert FIRRTL to Verilog |

## SEE ALSO

### Code Generation Guidelines

- All generated C++ files must be formatted with `make format` (clang‑format) and pass `make tidy` (clang‑tidy).
- Follow the **File Header Template** described in each skill for consistency.
- Refer to the `coding-style` SKILL for detailed formatting rules.

- **MAIN SKILL**: [Project overview](skill:main)
- **LLVM SKILL**: [LLVM mode details](skill:llvm)
- **HLS SKILL**: [High‑Level Synthesis mode](skill:hls)
- **CIRCT SKILL**: [CIRCT integration](skill:circt)
- **TESTING SKILL**: [Testing framework](skill:testing)