---
name: hls-refactoring
description: JLM HLS backend refactoring conventions. Use when refactoring HLS code, reviewing refactoring plans, or implementing new HLS transformations.
---

# JLM HLS Backend Refactoring Conventions

This skill covers the coding conventions and patterns used when refactoring the JLM HLS (High-Level Synthesis) backend.

## When to Activate

Use this skill when:
- Reviewing or creating refactoring plans for HLS code
- Implementing new transformations for the HLS backend
- Organizing existing HLS transformation files
- Debugging issues related to transformation pipeline integration
- Writing or updating HLS optimization passes

---

## Core Refactoring Principles

### 1. Transformations Extend rvsdg::Transformation

All HLS transformations must extend `rvsdg::Transformation` with this signature:

```cpp
class MyTransformation final : public rvsdg::Transformation {
public:
    ~MyTransformation() noexcept override;

    void Run(rvsdg::RvsdgModule& module, util::StatisticsCollector& stats) override;
};
```

**Key Points:**
- Use `rvsdg::Transformation` base class (don't create custom pipeline classes)
- Method signature: `Run(RvsdgModule&, StatisticsCollector&)`
- Include `noexcept override` in destructor

### 2. Use TransformationSequence for Pipelines

To run multiple transformations in sequence:

```cpp
auto transformation1 = std::make_shared<Transformation1>();
auto transformation2 = std::make_shared<Transformation2>();

std::vector<std::shared_ptr<rvsdg::Transformation>> sequence({
    transformation1,
    transformation2
});

return std::make_unique<rvsdg::TransformationSequence>(
    sequence, dotWriter, dumpRvsdgGraphs);
```

**Don't create:** Custom `PassPipeline` or `PassRegistry` classes

### 3. Error Handling with JLM_UNREACHABLE

Use `JLM_UNREACHABLE("message")` instead of exceptions:

```cpp
// Correct:
JLM_UNREACHABLE("Operation not implemented: " + node->DebugString());

// Wrong (don't use exceptions):
throw std::runtime_error("Error message");
```

**Why:** JLM uses `JLM_UNREACHABLE` (from `jlm/util/common.hpp`) for fatal errors that indicate programming bugs.

### 4. Test Modules Use llvm::LlvmRvsdgModule

HLS tests use `llvm::LlvmRvsdgModule`, not `rvsdg::RvsdgModule`:

```cpp
TEST(HlsTest, Example) {
    auto module = std::make_unique<llvm::LlvmRvsdgModule>(...);
    // Use llvm::LlvmRvsdgModule throughout tests
}
```

---

## Directory Organization
- **rvsdg2rhls/** – Transformations (one file per pass)
- **rhls2firrtl/** – FIRRTL generation (converters, emitters)


### rvsdg2rhls/ - Transformations

```
jlm/hls/backend/rvsdg2rhls/
├── passes/              # Organized transformation classes
│   ├── optimization/    # DNE, CNE-related
│   ├── conversion/      # Gamma, Theta, Memory conversions
│   ├── buffering/       # AddBuffers, AddForks, etc.
│   └── verification/    # R-HLS verification passes
└── rvsdg2rhls.cpp       # Main orchestrator (uses TransformationSequence)
```

**Pattern:** Each transformation is a separate file extending `rvsdg::Transformation`.

### rhls2firrtl/ - FIRRTL Generation

```
jlm/hls/backend/rhls2firrtl/
├── generators/          # Operation-specific generators
│   ├── GeneratorInterface.hpp
│   ├── GeneratorRegistry.hpp/cpp
│   └── *OpGenerator.hpp/cpp
└── RhlsToFirrtlConverter.cpp  # Uses generators (not giant switch)
```

**Pattern:** Use `JLM_UNREACHABLE` in generator methods for unimplemented operations.

---

## CNE Implementation Note

Each backend (LLVM, HLS) has its own CommonNodeElimination:

- **LLVM:** `jlm/llvm/opt/CommonNodeElimination.hpp`
- **HLS:** `jlm/hls/opt/cne.hpp`

**Don't try to share CNE between backends** - they have different requirements:
- LLVM handles phi nodes and complex control flow
- HLS focuses on LoopOperation and HLS-specific patterns

---

## Typical Refactoring Tasks

### 1. Creating a New Transformation

```cpp
// my-transformation.hpp
#ifndef JLM_HLS_BACKEND_RVSDG2RHLS_MYTRANSFORMATION_HPP
#define JLM_HLS_BACKEND_RVSDG2RHLS_MYTRANSFORMATION_HPP

#include <jlm/rvsdg/Transformation.hpp>

namespace jlm::hls {

class MyTransformation final : public rvsdg::Transformation {
public:
    ~MyTransformation() noexcept override;

    void Run(rvsdg::RvsdgModule& module, util::StatisticsCollector& stats) override;
};

}
#endif
```

```cpp
// my-transformation.cpp
#include "my-transformation.hpp"

namespace jlm::hls {

MyTransformation::~MyTransformation() noexcept = default;

void MyTransformation::Run(
    rvsdg::RvsdgModule& module,
    util::StatisticsCollector& stats)
{
    // Implementation here
}

}
```

### 2. Registering in Transformation Sequence

```cpp
#include "my-transformation.hpp"

// In createTransformationSequence():
auto myTransform = std::make_shared<MyTransformation>();
sequence.push_back(myTransform);

return std::make_unique<rvsdg::TransformationSequence>(
    sequence, dotWriter, dumpRvsdgGraphs);
```

### 3. Error Handling in Generators

```cpp
circt::firrtl::FModuleOp MyGenerator::Generate(
    RhlsToFirrtlConverter& converter,
    const rvsdg::Node* node)
{
    if (auto add = dynamic_cast<const llvm::IntegerAddOperation*>(&node->GetOperation())) {
        return HandleAdd(converter, node);
    }
    
    // For unimplemented operations:
    JLM_UNREACHABLE("Operation type not yet implemented: " + node->DebugString());
}
```

---

## Quick Reference
| Task | Action |
|------|--------|
| Create new transformation | Implement class extending `rvsdg::Transformation` and add to sequence |
| Register in pipeline | Push shared_ptr into `std::vector<std::shared_ptr<Transformation>>` |
| Error handling | Use `JLM_UNREACHABLE("msg")` |

-------

| Task | What to Do |
|------|------------|
| Create transformation | Extend `rvsdg::Transformation`, implement `Run(RvsdgModule&, StatisticsCollector&)` |
| Run multiple transforms | Use `rvsdg::TransformationSequence` with `std::vector<std::shared_ptr<Transformation>>` |
| Error handling | Use `JLM_UNREACHABLE("message")` |
| Test module type | `llvm::LlvmRvsdgModule` |
| File organization | Individual `.hpp/.cpp` per transformation in `rvsdg2rhls/` |
| CNE sharing | Don't share - each backend has its own |

---

## See Also

- **HLS TESTS**: [Unit testing framework](skill:hls-tests) - for writing tests
- **HLS**: [High-Level Synthesis mode](skill:hls) - for general HLS development
- **TESTING**: [Unit testing framework](skill:testing) - base jlm testing patterns