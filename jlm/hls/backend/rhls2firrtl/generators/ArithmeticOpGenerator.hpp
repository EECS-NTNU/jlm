/*
 * Copyright 2024 JLM contributors
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_HLS_BACKEND_RHLS2FIRRTL_GENERATORS_ARITHMETICOPGENERATOR_HPP
#define JLM_HLS_BACKEND_RHLS2FIRRTL_GENERATORS_ARITHMETICOPGENERATOR_HPP

#include "GeneratorInterface.hpp"
#include <jlm/rvsdg/node.hpp>

namespace jlm {
namespace hls {

class RhlsToFirrtlConverter;

} // namespace hls
} // namespace jlm

namespace jlm {
namespace hls {
namespace backend {
namespace rhls2firrtl {
namespace generators {

/// Generator for arithmetic operations: Add, Sub, And, Or, Xor
class ArithmeticOpGenerator final : public OperationGenerator {
public:
  ~ArithmeticOpGenerator() noexcept override = default;

  std::string
  name() const override {
    return "ArithmeticOpGenerator";
  }

  bool
  supports(const rvsdg::Operation & op) const override;

  void
  generate(const rvsdg::SimpleNode *node,
           RhlsToFirrtlConverter &converter) const override;
};

} // namespace generators
} // namespace rhls2firrtl
} // namespace backend
} // namespace hls
} // namespace jlm

#endif // JLM_HLS_BACKEND_RHLS2FIRRTL_GENERATORS_ARITHMETICOPGENERATOR_HPP