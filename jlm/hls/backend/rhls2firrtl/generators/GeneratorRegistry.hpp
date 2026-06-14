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