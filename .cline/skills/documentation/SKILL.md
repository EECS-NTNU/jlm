---
name: documentation
description: JLM compiler documentation generation. Use when working with Doxygen documentation, running `make docs`, or improving code documentation in the JLM project.
---

## Quick Cheat Sheet
- Generate docs: `make docs`
- Dry‑run format check (no changes): `make format-dry-run`
- Run clang‑tidy checks: `make tidy`

-------

This skill covers documenting the JLM compiler using Doxygen and generating HTML documentation.

## When Doxygen Documentation Is (and Isn't) Helpful

### ✅ Helpful For...

1. **Quick API Reference**: Understand function parameters, return values, and usage without reading implementation
2. **Design Intent**: Learn *why* code works a certain way from comments that explain rationale
3. **System Architecture**: Cross-referenced docs (`\see`, `\ref`) help map relationships between components
4. **Learning New Areas**: Well-documented classes serve as self-contained tutorials

### ❌ Less Helpful / Avoid...

1. **Redundant Comments**: Repeating function signatures (e.g., `/// \param x` where `x` is obvious from signature)
2. **Outdated Documentation**: Comments that don't match current behavior
3. **Over-documenting Trivial Code**: Simple getters/setters often need no documentation

### The High-Value Target for JLM

For the JLM compiler, focus documentation on:

| What to Document | Why It Matters |
|------------------|----------------|
| **Pipeline stages** (RVSDG → R-HLS → FIRRTL) | Understanding transformations between stages is non-trivial |
| **Critical design decisions** (e.g., BundleType purpose) | Explains *why* complex types exist |
| **API usage examples** | Shows how to use public interfaces correctly |
| **Class-level summaries** | Provides context before diving into implementation |

---

## Overview

The JLM project uses **Doxygen** to generate documentation from source code comments. The documentation is generated as HTML files and placed in the `docs/` directory.

## Generating Documentation

### Basic Command

```bash
make docs
```

This command:
1. Creates the `docs/` directory if it doesn't exist
2. Runs Doxygen with the configuration file `doxygen.conf`
3. Generates HTML documentation in `docs/html/`

### Clean and Regenerate

```bash
make docclean
make docs
```

The `docclean` target removes the existing `doc/` directory before regenerating.

## Documentation Configuration

### Configuration File

- **Location**: `doxygen.conf`
- **Format**: Key-value pairs with special syntax
- **Key Settings**:
  - `PROJECT_NAME = Jlm`
  - `OUTPUT_DIRECTORY = docs`
  - `INPUT` - Source directories to document
  - `EXCLUDE_PATTERNS` - Files/directories to skip

### Output Location

Documentation is generated at: `docs/html/`

Contains:
- Main index page (`index.html`)
- Classes and structs (organized alphabetically)
- Files and their contents
- Graphs (inheritance, collaboration diagrams)

## Doxygen Comment Syntax

The project uses **C-style multi-line comments** (`/** ... */`), not triple-slash style.

### Basic Comments

```cpp
/**
 * \brief Short description of a function.
 *
 * Detailed description spanning multiple lines.
 * Can include paragraphs and formatting.
 */
void myFunction(int name);
```

### For Classes

```cpp
/**
 * \brief Brief class description.
 *
 * Multi-line detailed description with:
 * - Features
 * - Usage notes
 * - Examples
 */
class MyClass {
public:
  /**
   * Constructor documentation.
   */
  MyClass();
};
```

### File Comments

```cpp
/**
 * \file
 * \brief Short description of file purpose.
 *
 * Longer description about what this file contains.
 */

#include "header.hpp"
```

## Comment Markers

### Standard Doxygen Tags

| Tag | Purpose |
|-----|---------|
| `\brief` | Short description (single line) - always start class/method docs with this |
| `\details` | Detailed description (when more context needed) |
| `\param name` | Function parameter documentation |
| `\return` | Return value documentation |
| `\tparam` | Template parameter documentation |
| `\ref` | Cross-reference to other documented items |
| `\see` | See also reference |
| `\sa` | See also (alternative to `\see`) |
| `\note` | Important note about behavior or requirements |

### Special Markers

```cpp
/**
 * \todo Fix this later - document future work needed
 * \deprecated Use newFunction() instead
 * \test Test coverage in test file
 */
```

## Code Examples in Documentation

### Inline Code

Wrap in backticks: `myVariable` or `#include "header.hpp"`

### Code Blocks

```
/// Example usage:
///
/// ```cpp
/// auto result = myFunction(42);
/// std::cout << result << std::endl;
/// ```
```

## Cross-References

### Referencing Other Documentation

```cpp
/// \see AnotherClass for similar functionality
/// \ref someFile.hpp "Related header file"
```

### Automatic Links

Doxygen automatically links to documented entities:
- Classes: `MyClass` → link to class documentation
- Functions: `myFunction()` → link to function documentation
- Files: `"header.hpp"` → link to file documentation

## Documenting HLS Backend

### R-HLS Conversion Pipeline

The HLS backend has three key transformation stages:

| Stage | File | Purpose |
|-------|------|---------|
| RVSDG→R-HLS | `rvsdg2rhls/` | Convert to Regional HLS representation |
| R-HLS→FIRRTL | `rhls2firrtl/` | Generate FIRRTL hardware description |
| FIRRTL→Verilog | `firrtl2verilog/` | Lower to Verilog for synthesis |

### Key Transformation Classes

#### GammaConversion
```cpp
/**
 * \brief Converts gamma nodes to HLS multiplexer operations.
 *
 * Gamma nodes in the RVSDG represent conditional control flow but need to be converted to
 * HLS-specific mux operations for hardware synthesis. This transformation is required before
 * FIRRTL generation and must run after gamma node creation but before the R-HLS conversion.
 */
```

#### ThetaConversion
```cpp
/**
 * \brief Converts theta nodes to HLS loop structures.
 *
 * Theta nodes in the RVSDG represent loops with carry variables. This transformation converts
 * them to HLS LoopNode representations suitable for hardware synthesis and FIRRTL generation.
 */
```

#### MemoryConverter
```cpp
/**
 * \brief Converts memory operations to explicit memory ports in the HLS representation.
 *
 * This transformation replaces implicit memory state edges with explicit request/response
 * bundles using BundleType. It's required for generating proper hardware interfaces (AXI, etc.)
 * in the FIRRTL backend.
 */
```

### BundleType Helper Functions

```cpp
/**
 * \brief Extracts memory response arguments from a kernel lambda node.
 *
 * Memory responses provide multiple values within a single execution of the region.
 * @param lambda The lambda node holding the HLS kernel.
 * @return Arguments representing memory responses (BundleType type).
 */
std::vector<rvsdg::RegionArgument *> get_mem_resps(const rvsdg::LambdaNode & lambda);

/**
 * \brief Extracts register arguments from a kernel lambda node.
 *
 * Register arguments represent kernel inputs that are not memory responses,
 * including kernel arguments, state types, and context variables.
 */
std::vector<rvsdg::RegionArgument *> get_reg_args(const rvsdg::LambdaNode & lambda);
```

## Documenting Test Files

### Unit Test Documentation

```cpp
/// \file
/// \brief Tests for GammaNode conversion to HLS mux operations
///
/// These tests verify the correct transformation of gamma nodes
/// (representing conditional control flow) into multiplexer-based
/// hardware implementations suitable for FIRRTL generation.
```

## Build Integration

### Documentation Check in CI

```bash
# Verify docs build without errors
make docs 2>&1 | tee docs.log

# Check for doxygen warnings
grep -i "warning" docs.log
```

### Adding New Source Files to Documentation

Update `doxygen.conf` INPUT setting:

```conf
INPUT = jlm \
        tools \
        tests
```

Or add specific subdirectories:

```conf
jlm/rvsdg \
jlm/llvm \
jlm/hls/backend \
```

## Best Practices

### 1. Use Correct Comment Style

The project uses `/** ... */` (C-style multi-line), NOT `///` style:
- ❌ Wrong: `/// \brief`
- ✅ Right: `/** \brief`

This is the most common mistake when adding documentation.

### 2. Document Public API First

Focus on:
- Header files in `include/` or public directories
- Exported functions and classes
- Public member functions

### 3. Keep Comments Up to Date

Regularly review documentation when modifying code.

### 4. Use Consistent Style

```cpp
/**
 * \brief One-line description.
 *
 * Multi-paragraph detailed description with:
 * - Bullet points using hyphens
 * - Cross-references with \see or \sa
 */
```

### 5. Document Edge Cases

```cpp
/**
 * \note This function returns empty string on error.
 * \warning Caller must check for null return values.
 */
```

## Common Issues

### Doxygen Not Finding Headers

```bash
# Add include path to doxygen.conf
INPUT += /path/to/headers
```

### Missing Cross-References

Ensure classes/functions are in documented directories:

```conf
EXCLUDE_PATTERNS = */test/*  # Exclude test files
```

### Graphs Not Generating

Enable graph generation:

```conf
HAVE_DOT = YES
CLASS_DIAGRAMS = YES
COLLABORATION_GRAPH = YES
```

## Quick Reference: JLM Documentation Workflow

### Verify Documentation Builds
```bash
make docs           # Generate docs in docs/html/
make docclean       # Remove generated docs
```

### When to Skip Documentation
1. Simple `= delete` copy/move constructors (obvious from context)
2. Obvious `extension()` methods returning string literals
3. Trivial helper functions where function name is self-explanatory

### Redundant Patterns to Avoid
| Pattern | Why Avoid | Alternative |
|---------|-----------|-------------|
| `/// \param x` when parameter name is obvious | Adds noise, not value | Omit or add context in description |
| Multi-line docs for simple getters/setters | Overkill for trivial code | Brief `\brief` only if at all |
| Repeating function signature info | Code shows this already | Document behavior/rationale |

## See Also
- **MAIN SKILL**: [Project overview](skill:main)
- **HLS SKILL**: [High-Level Synthesis mode](skill:hls)
- **MLIR SKILL**: [MLIR mode details](skill:mlir)


- **MAIN SKILL**: [Project overview](skill:main)
- **HLS SKILL**: [High-Level Synthesis mode](skill:hls)
- **TESTING SKILL**: [Unit testing framework](skill:testing)
