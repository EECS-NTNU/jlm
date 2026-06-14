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