# RVSDG Skill

## Quick Overview
The **RVSDG (Region‑based Value State Dependence Graph)** is the core intermediate representation used throughout JLM. It models programs as a graph of regions, nodes, and operations, enabling sophisticated optimizations and transformations.

## Core Concepts
| Concept | Description |
|---------|-------------|
| **Graph** | Container for all regions and nodes in a compilation unit (`jlm/rvsdg/graph.cpp`). |
| **Region** | Hierarchical block with its own entry/exit arguments. Used to represent control flow constructs (loops, conditionals). |
| **Node** | Represents an operation or value; can be simple (`binary`, `unary`) or complex (`gamma`, `theta`). |
| **Operation** | The actual computation performed by a node (e.g., add, mul, call). |
| **Phi / Theta** | Special nodes for merging values across control flow. |

## Important Files & Directories
- `jlm/rvsdg/` – implementation of the IR.
- `jlm/rvsdg/*.cpp/.hpp` – concrete node and operation definitions.
- `jlm/rvsdg/tests/` – unit‑tests exercising the IR (e.g., `GammaTests.cpp`, `ThetaTests.cpp`).

## Common Workflows
1. **Create a new graph**  
   ```cpp
   RvsdgModule module;
   Graph &g = module.graph();
   ```
2. **Add a region**  
   ```cpp
   Region *r = g.create_region(...);
   ```
3. **Insert nodes/operations**  
   ```cpp
   auto *add = r->emplace<BinaryNode>(op::ADD, lhs, rhs);
   ```
4. **Connect arguments & results** – use `connect` or `set_result`.

## Adding New Nodes / Operations
1. Add header/source under `jlm/rvsdg/`.  
2. Derive from `NodeBase` and implement `operation()` if needed.  
3. Register the node in `RvsdgModule` (optional).  
4. Write unit‑tests in `jlm/rvsdg/tests/`.

## Building & Testing
```bash
# Build core RVSDG library
make -C jlm/rvsdg

# Run all RVSDG tests
make -C jlm/rvsdg test   # or ./scripts/run-llvm-test-suite.sh (filters RVSDG)
```

## Frequently Used Commands
| Action | Command |
|--------|---------|
| Build the whole project | `make` |
| Re‑run only RVSDG unit tests | `make -C jlm/rvsdg test` |
| Generate compile commands for IDE | `bear -- make` |

## See Also

### Code Generation Guidelines

- All generated C++ files must be formatted with `make format` (clang‑format) and pass `make tidy` (clang‑tidy).
- Follow the **File Header Template** described in each skill for consistency.
- Refer to the `coding-style` SKILL for detailed formatting rules.

- `.cline/skills/main/SKILL.md` – overall JLM workflow.  
- `.cline/skills/testing/SKILL.md` – testing conventions.  

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

*This skill provides a concise cheat‑sheet for developers working with the RVSDG IR.*