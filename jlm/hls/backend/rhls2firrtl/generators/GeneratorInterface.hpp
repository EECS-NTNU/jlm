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