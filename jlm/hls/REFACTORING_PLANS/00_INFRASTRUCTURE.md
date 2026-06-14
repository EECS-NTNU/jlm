# 00 INFRASTRUCTURE – Generator Architecture

## Overview
The HLS backend currently mixes conversion logic directly inside `RhlsToFirrtlConverter.cpp`.  
This document defines a **modular generator infrastructure** that will:

1. Provide a clear, stable API for all operation generators.
2. Allow thread‑safe registration and lookup of generators.
3. Enable incremental migration from the monolithic converter to a registry‑based design.
4. Facilitate isolated unit testing of each generator.

All code changes in later phases must respect the contracts described here.

---

## 1️⃣ OperationGenerator Interface
*File:* `jlm/hls/backend/rhls2firrtl/generators/GeneratorInterface.hpp`

```cpp
#ifndef JLM_HLS_BACKEND_RHLS2FIRRTL_GENERATORS_GENERATORINTERFACE_HPP
#define JLM_HLS_BACKEND_RHLS2FIRRTL_GENERATORS_GENERATORINTERFACE_HPP

#include <string>
#include "jlm/hls/backend/rhls2firrtl/base-hls.hpp"

namespace jlm {
namespace hls {
namespace backend {
namespace rhls2firrtl {
namespace generators {

/// Abstract base class for all operation generators.
/// Each generator translates a specific HLS Operation into FIRRTL
/// using the provided FirrtlBuilder. Implementations must be
/// stateless or internally synchronized.
class OperationGenerator {
public:
  virtual ~OperationGenerator() = default;

  /// Human‑readable name used for registration and diagnostics.
  virtual std::string name() const = 0;

  /// Generate FIRRTL code for @p op using @p builder.
  virtual void generate(const Operation &op,
                        FirrtlBuilder &builder) const = 0;

  /// Optional predicate indicating whether this generator can handle
  /// the supplied operation. Default returns true; override only when a
  /// generator supports a subset of operations.
  virtual bool supports(const Operation &op) const { return true; }
};

} // namespace generators
} // namespace rhls2firrtl
} // namespace backend
} // namespace hls
} // namespace jlm

#endif // JLM_HLS_BACKEND_RHLS2FIRRTL_GENERATORS_GENERATORINTERFACE_HPP
```

*Key points*
- `name()` must be unique across all generators.
- Implementations should **never** modify global state; any needed caches must be protected internally.

---

## 2️⃣ GeneratorRegistry
*File:* `jlm/hls/backend/rhls2firrtl/generators/GeneratorRegistry.hpp`  
*Implementation:* `GeneratorRegistry.cpp`

```cpp
#ifndef JLM_HLS_BACKEND_RHLS2FIRRTL_GENERATORS_REGISTRY_HPP
#define JLM_HLS_BACKEND_RHLS2FIRRTL_GENERATORS_REGISTRY_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <vector>

#include "GeneratorInterface.hpp"

namespace jlm {
namespace hls {
namespace backend {
namespace rhls2firrtl {
namespace generators {

/// Thread‑safe singleton registry for OperationGenerators.
class GeneratorRegistry {
public:
  /// Returns the global instance (Meyers singleton).
  static GeneratorRegistry &instance();

  /// Register a new generator; takes ownership of @p gen.
  void registerGenerator(std::unique_ptr<OperationGenerator> gen);

  /// Retrieve a generator by its name. Returns nullptr if not found.
  OperationGenerator *get(const std::string &name) const;

  /// List all registered generator names (useful for diagnostics).
  std::vector<std::string> listGenerators() const;

private:
  GeneratorRegistry() = default;
  ~GeneratorRegistry() = default;
  GeneratorRegistry(const GeneratorRegistry &) = delete;
  GeneratorRegistry &operator=(const GeneratorRegistry &) = delete;

  mutable std::shared_mutex mutex_;
  std::unordered_map<std::string, std::unique_ptr<OperationGenerator>> generators_;
};

/// Convenience macro for registration in a translation unit.
#define REGISTER_GENERATOR(GenType)                                   \
  namespace {                                                         \
  struct GenType##Registrar {                                         \
    GenType##Registrar() {                                            \
      jlm::hls::backend::rhls2firrtl::generators::GeneratorRegistry   \
          ::instance()                                                \
          .registerGenerator(std::make_unique<GenType>());            \
    }                                                                 \
  };                                                                  \
  static GenType##Registrar global_##GenType##_registrar;             \
  }

} // namespace generators
} // namespace rhls2firrtl
} // namespace backend
} // namespace hls
} // namespace jlm

#endif // JLM_HLS_BACKEND_RHLS2FIRRTL_GENERATORS_REGISTRY_HPP
```

**Implementation (`GeneratorRegistry.cpp`)**

```cpp
#include "GeneratorRegistry.hpp"

namespace jlm {
namespace hls {
namespace backend {
namespace rhls2firrtl {
namespace generators {

GeneratorRegistry &GeneratorRegistry::instance() {
  static GeneratorRegistry instance;
  return instance;
}

void GeneratorRegistry::registerGenerator(std::unique_ptr<OperationGenerator> gen) {
  if (!gen)
    return;
  std::unique_lock lock(mutex_);
  const auto name = gen->name();
  generators_.emplace(name, std::move(gen));
}

OperationGenerator *GeneratorRegistry::get(const std::string &name) const {
  std::shared_lock lock(mutex_);
  auto it = generators_.find(name);
  return (it != generators_.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> GeneratorRegistry::listGenerators() const {
  std::shared_lock lock(mutex_);
  std::vector<std::string> names;
  names.reserve(generators_.size());
  for (const auto &pair : generators_)
    names.push_back(pair.first);
  return names;
}

} // namespace generators
} // namespace rhls2firrtl
} // namespace backend
} // namespace hls
} // namespace jlm
```

*Design notes*
- **Singleton** ensures a single source of truth; registration happens at static‑initialization time via `REGISTER_GENERATOR`.
- **Thread safety:** writes (registration) acquire an exclusive lock; lookups use shared locks for concurrent reads.
- Missing generators result in `nullptr`; callers should fall back to emitting `JLM_UNREACHABLE` with a clear error message.

---

## 3️⃣ Migration Strategy

| Step | Action |
|------|--------|
| **3.1** | Scan the existing `RhlsToFirrtlConverter.cpp` for conversion functions tied to specific operations. |
| **3.2** | For each operation, create a concrete class inheriting from `OperationGenerator`. Implement `name()`, `generate(...)`, and optionally `supports(...)`. |
| **3.3** | Add `REGISTER_GENERATOR(MyOpGen)` at the bottom of the new source file so it is auto‑registered. |
| **3.4** | Replace direct calls in `RhlsToFirrtlConverter.cpp` with:<br>`if (auto *gen = GeneratorRegistry::instance().get(op.name())) gen->generate(op, builder); else JLM_UNREACHABLE("No generator for " + op.name());` |
| **3.5** | Run the baseline test suite after each batch of migrations to catch regressions early (see Task 3 below). |
| **3.6** | Update CI (`hls_refactor_ci.yml`) to compile the new `generators/` directory. |

---

## 4️⃣ Checklist for Phase 0

- [ ] Draft `00_INFRASTRUCTURE.md` (this file) and get team sign‑off.
- [ ] Create directory `jlm/hls/backend/rhls2firrtl/generators/`.
- [ ] Add `GeneratorInterface.hpp`, `GeneratorRegistry.hpp`, `GeneratorRegistry.cpp` with the contents above.
- [ ] Verify the files compile (`make -C jlm/hls` or equivalent) after adding them.
- [ ] Run existing HLS unit tests to ensure no regression introduced by the new headers.
- [ ] Commit the changes on a dedicated branch (e.g., `infra-generator-setup`) and open a PR.

---

## 5️⃣ FAQ

**Q: Why use a singleton instead of passing the registry?**  
A: The registry is required by many conversion passes. A global accessor avoids plumbing the object through numerous function signatures while still providing controlled access.

**Q: Will the mutex be a performance bottleneck?**  
A: Registration happens once at start‑up. Runtime lookups are read‑only and use a shared lock, which has negligible overhead compared to the actual code generation work.

**Q: How do I add a new generator?**  
A: Implement a class derived from `OperationGenerator`, place it in `jlm/hls/backend/rhls2firrtl/generators/`, and include `REGISTER_GENERATOR(MyGen);` at the end of the source file.

---

*All further refactoring phases (RVSDG→RHLS, RHLS→FIRRTL, optimization passes, etc.) must build on this infrastructure.*