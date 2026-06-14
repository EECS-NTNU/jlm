# Util Skill

## Quick Reference
The **util** package contains a collection of reusable C++ utilities that support the JLM compiler infrastructure. These helpers are lightweight, header‑only or small compiled libraries and are widely used across all compiler components.

## Major Utility Modules
| Module | Primary Purpose |
|--------|-----------------|
| `AnnotationMap` | Associates arbitrary metadata with IR objects (e.g., nodes). |
| `GraphWriter`   | Serialises graphs (RVSDG, MLIR) to DOT for visualisation. |
| `Worklist`      | Simple priority‑free work‑list implementation for iterative algorithms. |
| `HashSet / HashMap` | High‑performance hash containers with custom hashing support. |
| `Program`       | Helper for invoking external programs and capturing output. |
| `Timer`         | Lightweight timing utilities for benchmarking passes. |
| `Statistics`    | Accumulates statistical data (counters, histograms). |

## Typical Usage Patterns

### AnnotationMap
```cpp
AnnotationMap<std::string> ann;
ann.set(node, "origin", "frontend");
auto val = ann.get(node, "origin"); // -> "frontend"
```

### GraphWriter
```bash
# Generate DOT for a given RVSDG graph (binary):
./tools/jlc/jlc-opt -dot <input.rvsdg> -o out.dot
dot -Tpng out.dot -o graph.png
```
Or programmatically:
```cpp
GraphWriter writer;
writer.write(g, "graph.dot");
```

### Worklist
```cpp
Worklist<Node*> wl;
wl.push(node);
while (!wl.empty()) {
  Node *n = wl.pop();
  // process n …
}
```

## Building & Testing Utilities
All utilities are compiled as part of the top‑level `make` target. To focus on util tests only:
```bash
make -C jlm/util test   # runs all *.Tests.cpp in this directory
```

## Frequently Used Commands
| Goal | Command |
|------|---------|
| Build only utils | `make -C jlm/util` |
| Run util unit tests | `make -C jlm/util test` |
| Generate documentation for utils | `doxygen doxygen.conf && open docs/html/index.html` |

## See Also
- `.cline/skills/testing/SKILL.md` – testing conventions used by util tests.  
- `.cline/skills/main/SKILL.md` – overall project build flow.

--- 

*The Util skill offers a concise cheat‑sheet for developers needing quick access to JLM’s helper libraries.*