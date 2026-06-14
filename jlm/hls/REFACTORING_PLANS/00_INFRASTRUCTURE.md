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
};

} // namespace jlm::hls::rhls2firrtl
```

### Implementation Guidelines
* **Stateless:** Generators should not store mutable state; any required configuration is passed via constructor arguments.
* **Error handling:** Use `JLM_UNREACHABLE` (defined in `jlm/util/common.hpp`) for impossible code paths rather than throwing exceptions.
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

## 3️⃣ Migration Strategy – `runtime_error` → `JLM_UNREACHABLE`

Existing code frequently uses:

```cpp
throw std::runtime_error("unreachable");
```

The project now prefers the macro defined in `jlm/util/common.hpp`:

```cpp
#define JLM_UNREACHABLE \
  ::jlm::util::unreachable(__FILE__, __LINE__)
```

### Steps

1. **Search & Replace** – Replace all occurrences of `throw std::runtime_error(` with `JLM_UNREACHABLE;`.
2. **Compile‑time Flag** – Introduce `ENABLE_LEGACY_EXCEPTIONS` to temporarily allow legacy code during the migration wave.
3. **Gradual Removal** – After each module passes its unit tests, disable the flag for that module and remove any leftover includes of `<stdexcept>`.
4. **Verification** – Run the full test suite (`make check`) after each batch of replacements.

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

- [ ] Implement **OperationGenerator** interface (already present).
- [ ] Implement **GeneratorRegistry** singleton with thread safety.
- [ ] Write concrete generators for at least the arithmetic and memory opcodes as proof‑of‑concept.
- [ ] Add registration code (`registerAllGenerators`) and invoke it during backend initialization.
- [ ] Migrate all `runtime_error` usages to `JLM_UNREACHABLE`.
- [ ] Update `jlm/hls/Makefile.sub` and CI workflow to compile new sources.
- [ ] Document each generator and the registry according to the guidelines above.

*All modifications are confined to `jlm/hls/` as required by the overall refactoring plan.*