# Phase 0 – Infrastructure

## Purpose
This document defines the core infrastructure that must be in place before any HLS backend refactoring work proceeds. All later phases (RVSDG→RHLS, RHLS→FIRRTL, optimisation passes, etc.) depend on these components being available, correctly built, and documented.

---

## 1️⃣ `OperationGenerator` Interface

The **OperationGenerator** is a pure‑virtual base class that encapsulates the conversion of a single RHLS node into FIRRTL using a builder object. Every concrete generator (arithmetic, memory, control‑flow, structural) inherits from this interface.

```cpp
// jlm/hls/backend/rhls2firrtl/generators/GeneratorInterface.hpp
#pragma once

#include "rhls/Node.hpp"
#include "firrtl/Builder.hpp"
#include <vector>
#include <stdexcept>

namespace jlm::hls::rhls2firrtl {

class OperationGenerator {
public:
  virtual ~OperationGenerator() = default;

  /// Convert the given RHLS node to FIRRTL.
  ///
  /// @param node   The RHLS node to be translated.
  /// @param builder The FIRRTL builder that accumulates generated operations.
  virtual void generate(const rhls::Node *node,
                        firrtl::Builder &builder) const = 0;

  /// Human‑readable name used for diagnostics and error messages.
  virtual std::string name() const = 0;

  /// List of supported operation names for diagnostic purposes.
  virtual std::vector<const char*> getSupportedOperations() const = 0;
};

} // namespace jlm::hls::rhls2firrtl
```

### Implementation Guidelines
* **Stateless:** Generators should not store mutable state; any required configuration is passed via constructor arguments.
* **Error handling during migration:** Use `std::runtime_error` with helpful messages listing supported operations. Once all operations are implemented, switch to `JLM_UNREACHABLE`.
* **Testing:** Each generator must have a dedicated unit test that builds a minimal RHLS fragment and checks the generated FIRRTL.

---

## 2️⃣ Thread‑Safe `GeneratorRegistry`

The registry maps an RHLS opcode to its corresponding `OperationGenerator`. It is globally accessible via a singleton and protects registration/lookup with a mutex, allowing concurrent compilation of different translation units.

```cpp
// jlm/hls/backend/rhls2firrtl/generators/GeneratorRegistry.hpp
#pragma once

#include "GeneratorInterface.hpp"
#include <unordered_map>
#include <memory>
#include <mutex>

namespace jlm::hls::rhls2firrtl {

class GeneratorRegistry {
public:
  static GeneratorRegistry &instance();

  /// Register a generator for the given opcode.
  ///
  /// @param op   The RHLS opcode (e.g. rhls::Opcode::Add)
  /// @param gen  A unique_ptr to a concrete OperationGenerator.
  void registerGenerator(rhls::Opcode op,
                         std::unique_ptr<OperationGenerator> gen);

  /// Retrieve the generator for an opcode. Throws `std::runtime_error`
  /// if the opcode has not been registered.
  const OperationGenerator &getGenerator(rhls::Opcode op) const;

private:
  GeneratorRegistry() = default;
  mutable std::mutex mtx_;
  std::unordered_map<rhls::Opcode, std::unique_ptr<OperationGenerator>> map_;
};

} // namespace jlm::hls::rhls2firrtl
```

The accompanying implementation (`GeneratorRegistry.cpp`) provides the thread‑safe logic.

### Registration Pattern

All generators are registered in a dedicated source file (e.g. `register_all_generators.cpp`). Example:

```cpp
#include "GeneratorRegistry.hpp"
#include "ArithmeticGenerator.hpp"
// ... other generator includes ...

void registerAllGenerators() {
  auto &reg = GeneratorRegistry::instance();
  reg.registerGenerator(rhls::Opcode::Add,
                       std::make_unique<ArithmeticGenerator>());
  // repeat for each opcode …
}
```

`registerAllGenerators()` is called from the backend’s initialization routine before any conversion starts.

---

## 3️⃣ Error Handling Strategy

### For Operation Generators (Refactoring Phase)

During refactoring, use `std::runtime_error` for missing generators to provide helpful error messages:

```cpp
class GeneratorRegistry {
public:
  const OperationGenerator &getGenerator(rhls::Opcode op) const {
    auto it = map_.find(op);
    if (it == map_.end()) {
      std::string msg = "No generator registered for opcode: ";
      // Include list of supported operations in error message
      throw std::runtime_error(msg + getOperationName(op));
    }
    return *(it->second);
  }
};
```

**Benefits:**
- Graceful error handling (no compiler crash)
- Helpful messages listing available generators
- Developer-friendly during migration phase

### For Production Code (After Migration Complete)

Once all operations have generators, switch to `JLM_UNREACHABLE` for truly impossible code paths:

```cpp
// Only use JLM_UNREACHABLE when ALL operations are implemented
#define JLM_UNREACHABLE \
  ::jlm::util::unreachable(__FILE__, __LINE__)
```

### Migration Path

This error handling strategy applies to **all phases that introduce new operation generators**:

1. **Phase 0**: Implement generator interface with `runtime_error` for missing operations
   - All generators throw `std::runtime_error` when called with unimplemented operations
   - Error message includes list of supported operations via `GetSupportedOperations()`

2. **Phases 3-7**: Extract existing operations to generators; use `runtime_error` during transition
   - Both old MlirGen* methods and new generators can coexist
   - Use runtime_error until ALL operations have corresponding generators

3. **Final step**: After all ops have generators, add JLM_UNREACHABLE for truly impossible cases
   - Only use JLM_UNREACHABLE when the operation itself is unimplemented in the RHLS IR
   - This is different from "no generator registered yet" which uses runtime_error

#### When to Use Each Error Type

| Scenario | Error Type | Rationale |
|----------|------------|-----------|
| Generator called for unsupported operation during migration | `std::runtime_error` | Graceful failure, helpful error message |
| Generator called for truly unimplemented RHLS operation | `JLM_UNREACHABLE` | Indicates bug in RHLS IR design |
| Internal invariant violation in generator | `JLM_ASSERT` or `JLM_UNREACHABLE` | Programming error |

**Note**: This approach is consistent with Phase 3 (rhls2firrtl) which specifies runtime_error during the migration phase.

#### Migration Checklist

Before switching from runtime_error to JLM_UNREACHABLE, verify:

1. All operations in RhlsToFirrtlConverter.cpp have corresponding generators
2. Test suite passes with all new generators registered
3. No runtime_error exceptions thrown during full pipeline execution
4. Code review confirms no unhandled operation cases remain

---

## 4️⃣ Build Integration

* Add the `generators/` directory to the HLS backend source list in `jlm/hls/Makefile.sub`:

```make
HLS_SRCS += \
    $(JLM_ROOT)/hls/backend/rhls2firrtl/generators/GeneratorInterface.cpp \
    $(JLM_ROOT)/hls/backend/rhls2firrtl/generators/GeneratorRegistry.cpp \
    # plus any concrete generator .cpp files
```

* Ensure the include path `-I$(JLM_ROOT)/hls/backend/rhls2firrtl` is present.
* Update CI workflow `.github/workflows/hls_refactor_ci.yml` to run a dry‑build after the registry changes:

```yaml
- name: Build HLS backend
  run: |
    cd jlm/hls
    make -f Makefile.sub
```

---

## 5️⃣ Documentation Guidelines

* Every concrete generator must have a Doxygen comment block describing:
  * The RHLS operation(s) it handles.
  * Any FIRRTL‑specific constraints.
  * Example usage (optional).
* Add a high‑level overview entry in this `00_INFRASTRUCTURE.md` linking to each generator’s documentation file.
* The registry registration source (`register_all_generators.cpp`) should also be documented with a brief module description.

---

## 6️⃣ Checklist for Phase 0 Completion

- [ ] Implement **OperationGenerator** interface with runtime_error migration strategy
- [ ] Implement **GeneratorRegistry** singleton with thread safety
- [ ] Write concrete generators for at least the arithmetic and memory opcodes as proof‑of‑concept
- [ ] Add registration code (`registerAllGenerators`) and invoke it during backend initialization
- [ ] Create local verification scripts (`./scripts/verify-baseline.sh`, `./scripts/verify-local.sh`)
- [ ] Update `jlm/hls/Makefile.sub` to compile new sources (local alternative to CI)

*All modifications are confined to `jlm/hls/` as required by the overall refactoring plan.*
