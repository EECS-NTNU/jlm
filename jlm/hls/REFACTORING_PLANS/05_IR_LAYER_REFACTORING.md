# Phase 5: HLS IR Layer Refactoring Plan

## Document Information
- **Phase**: 5 (IR Layer)
- **Priority**: MEDIUM - Foundation layer
- **Estimated Duration**: 1 week
- **Status**: Planning Phase

---

## 1. Current Architecture Analysis

### HLS IR Components

```
jlm/hls/ir/
├── hls.hpp    # Main header with all HLS types (~1760 lines!)
└── hls.cpp    # Implementation
```

### Key Issues Identified

1. **hls.hpp is 1760 lines** - all HLS operations in one file
2. **Mixed concerns** - Types, operations, node classes all mixed
3. **Limited documentation** - Some classes lack Doxygen comments
4. **Circular dependencies potential** - Forward declarations scattered

### Current hls.hpp Organization

```cpp
// Currently organized by type (not by concern)
├── Type Definitions          // ~200 lines
│   ├── BundleType
│   └── TriggerType
│
├── Simple Operations         // ~500 lines
│   ├── BranchOperation
│   ├── ForkOperation
│   ├── MuxOperation
│   ├── SinkOperation
│   ├── BufferOperation
│   ├── PrintOperation
│   └── others...
│
├── Structural Operations     // ~300 lines
│   ├── LoopOperation
│   └── LoopNode
│
└── Helper Functions          // ~760 lines
    ├── get_mem_req_type
    ├── JlmSize
    └── others...
```

---

## 2. Refactoring Goals

### Phase 5.1: Module Division

#### Goal
Split hls.hpp into focused modules:
- One file per concern area
- Clear module dependencies
- Consistent naming and organization

#### New Directory Structure
```
jlm/hls/ir/
├── types/                    # NEW: Type definitions
│   ├── BundleType.hpp/cpp
│   └── TriggerType.hpp/cpp
├── operations/               # NEW: Operation classes
│   ├── BranchOperation.hpp/cpp
│   ├── ForkOperation.hpp/cpp
│   ├── MuxOperation.hpp/cpp
│   ├── BufferOperation.hpp/cpp
│   └── ControlOperations.hpp/cpp  # Grouped control ops
├── nodes/                    # NEW: Node classes
│   ├── LoopNode.hpp/cpp
│   └── others...
└── hls.hpp                  # Main header (reduced, includes modules)
```

### Phase 5.2: Documentation Improvement

#### Goals
1. Add Doxygen documentation to all public classes
2. Document operation semantics
3. Add usage examples for complex types

---

## 3. Detailed Design

### 3.1 Types Module

```cpp
// ir/types/BundleType.hpp
/**
 * \brief Bundle type for HLS signal aggregation.
 *
 * Bundle types group multiple signals together (e.g., ready/valid pairs).
 * Each bundle has a name and type for each element.
 */
class BundleType final : public rvsdg::Type {
public:
    /**
     * Create a new bundle type with the given elements.
     * @param elements Vector of (name, type) pairs
     */
    explicit BundleType(const std::vector<std::pair<std::string, std::shared_ptr<const Type>>> elements);
    
    // ... rest of interface ...
};

// ir/types/TriggerType.hpp
/**
 * \brief Trigger type for HLS synchronization.
 *
 * Trigger types are used for control flow coordination in HLS circuits.
 */
class TriggerType final : public rvsdg::Type {
    // ... interface ...
};
```

### 3.2 Operations Module

```cpp
// ir/operations/BranchOperation.hpp
/**
 * \brief Branch operation for conditional value selection.
 *
 * The branch operation selects one of multiple alternatives based on a control signal.
 */
class BranchOperation final : public rvsdg::SimpleOperation {
    // ... interface ...
};

// ir/operations/ForkOperation.hpp
/**
 * \brief Fork operation for signal fanout handling.
 *
 * Fork operations handle the case where one producer feeds multiple consumers,
 * ensuring proper handshaking (ready/valid) semantics.
 */
class ForkOperation final : public rvsdg::SimpleOperation {
    // ... interface ...
};

// ir/operations/ControlOperations.hpp
/**
 * \brief Group of control-related HLS operations.
 *
 * Contains: Branch, Mux, Buffer, Print, Trigger
 */
namespace hls {
    // Include all control ops in one header for convenience
}
```

### 3.3 Nodes Module

```cpp
// ir/nodes/LoopNode.hpp
/**
 * \brief HLS loop node representing structured loops.
 *
 * Loop nodes represent iterative computation patterns in HLS.
 * They handle:
 * - Loop-carried variables (back-edges)
 * - Predicate buffer for iteration control
 * - Loop constant buffering
 */
class LoopNode final : public rvsdg::StructuralNode {
    // ... interface ...
};
```

### 3.4 Main Header Organization

```cpp
// ir/hls.hpp
#ifndef JLM_HLS_IR_HLS_HPP
#define JLM_HLS_IR_HLS_HPP

// Include module headers
#include <jlm/hls/ir/types/BundleType.hpp>
#include <jlm/hls/ir/types/TriggerType.hpp>
#include <jlm/hls/ir/operations/BranchOperation.hpp>
#include <jlm/hls/ir/operations/ForkOperation.hpp>
#include <jlm/hls/ir/nodes/LoopNode.hpp>

// Re-export main namespace aliases
namespace jlm::hls {
    // Convenience typedefs and functions
}

#endif  // JLM_HLS_IR_HLS_HPP
```

---

## 4. Implementation Tasks

### Task 5.1: Create Module Directories

- [ ] Create `ir/types/` directory
- [ ] Create `ir/operations/` directory  
- [ ] Create `ir/nodes/` directory

**Deliverable**: New directory structure ready

### Task 5.2: Extract Types Module

- [ ] Move BundleType to `types/BundleType.hpp/cpp`
- [ ] Move TriggerType to `types/TriggerType.hpp/cpp`
- [ ] Add Doxygen documentation
- [ ] Create tests for type operations

**Test Coverage**: Type comparison, hash computation, debug strings

### Task 5.3: Extract Operations Module

Create organized operation headers:

| File | Contents |
|------|----------|
| `operations/BranchOperation.hpp` | BranchOperation class |
| `operations/ForkOperation.hpp` | ForkOperation class |
| `operations/MuxOperation.hpp` | MuxOperation class |
| `operations/ControlOperations.hpp` | Bundle of control ops |
| `operations/BufferOperation.hpp` | BufferOperation class |

**Test Coverage**: Operation equality, copy, hash for each

### Task 5.4: Extract Nodes Module

- [ ] Move LoopNode to `nodes/LoopNode.hpp/cpp`
- [ ] Add Doxygen documentation
- [ ] Create tests for node operations

**Test Coverage**: Node creation, loop variable management

### Task 5.5: Update Main Header

- [ ] Refactor hls.hpp to include module headers
- [ ] Keep only essential inline functions
- [ ] Update all includes in the project

### Task 5.6: Documentation Updates

- [ ] Add Doxygen comments to all classes
- [ ] Document operation semantics
- [ ] Add usage examples for complex patterns

---

## 5. Test Strategy for Phase 5

### Type Tests
```cpp
// ir/types/BundleTypeTests.cpp
TEST(BundleType, Equality) {
    auto type1 = BundleType::Create({{"data", IntType}, {"valid", BoolType}});
    auto type2 = BundleType::Create({{"data", IntType}, {"valid", BoolType}});
    
    EXPECT_EQ(*type1, *type2);
}

TEST(BundleType, ElementAccess) {
    auto type = BundleType::Create({{"a", TypeA}, {"b", TypeB}});
    
    EXPECT_EQ(type->get_element_type("a"), TypeA);
    EXPECT_EQ(type->get_element_type("b"), TypeB);
}
```

### Operation Tests
```cpp
// ir/operations/BranchOperationTests.cpp
TEST(BranchOperation, Copy) {
    auto op = BranchOperation::Create(predicate, value);
    auto copy = op->copy();
    
    EXPECT_TRUE(*op == *copy);
}

TEST(BranchOperation, Hash) {
    auto op1 = BranchOperation::Create(predicate, value);
    auto op2 = BranchOperation::Create(predicate, value);
    
    EXPECT_EQ(op1->ComputeHash(), op2->ComputeHash());
}
```

### Node Tests
```cpp
// ir/nodes/LoopNodeTests.cpp
TEST(LoopNode, AddLoopVariable) {
    auto region = CreateTestRegion();
    auto loopNode = LoopNode::create(region);
    
    auto input = CreateTestOutput();
    auto output = loopNode->AddLoopVar(input);
    
    EXPECT_NE(output, nullptr);
}
```

---

## 6. Files to Create/Modify

### New Files to Create
| File | Purpose |
|------|---------|
| `ir/types/BundleType.hpp/cpp` | Bundle type implementation |
| `ir/types/TriggerType.hpp/cpp` | Trigger type implementation |
| `ir/operations/BranchOperation.hpp/cpp` | Branch operation |
| `ir/operations/ForkOperation.hpp/cpp` | Fork operation |
| `ir/operations/MuxOperation.hpp/cpp` | Mux operation |
| `ir/nodes/LoopNode.hpp/cpp` | Loop node class |
| Tests for each module |

### Existing Files to Modify
| File | Change |
|------|--------|
| `ir/hls.hpp` | Refactor to include modules |
| `ir/hls.cpp` | Update includes |

---

## 7. Success Criteria

### Phase 5 Completion Checklist
- [ ] All types moved to module files
- [ ] All operations moved to module files
- [ ] All nodes moved to module files
- [ ] Main header reduced significantly (< 200 lines)
- [ ] Doxygen documentation added for all public classes
- [ ] Tests for each module pass

### Quality Metrics
| Metric | Target |
|--------|--------|
| Lines per module file | < 300 lines |
| Test coverage per module | > 80% |
| Documentation completeness | 100% |

---

## 8. Next Steps After Phase 5

1. Update all internal code to use new module includes
2. Add API documentation for public interface
3. Consider adding more helper utilities

---

*This phase organizes the HLS IR layer into well-defined modules with clear responsibilities.*