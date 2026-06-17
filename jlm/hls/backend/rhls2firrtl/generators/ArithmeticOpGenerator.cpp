/*
 * Copyright 2024 JLM contributors
 * See COPYING for terms of redistribution.
 */

#include <jlm/hls/backend/rhls2firrtl/generators/ArithmeticOpGenerator.hpp>

#include <jlm/hls/backend/rhls2firrtl/RhlsToFirrtlConverter.hpp>
#include <jlm/llvm/ir/operators/IntegerOperations.hpp>

namespace jlm {
namespace hls {
namespace backend {
namespace rhls2firrtl {
namespace generators {

bool
ArithmeticOpGenerator::supports(const rvsdg::Operation & op) const {
  auto debugStr = op.debug_string();
  return debugStr == "IntegerAdd" || debugStr == "IntegerSub"
      || debugStr == "IntegerAnd" || debugStr == "IntegerOr"
      || debugStr == "IntegerXor";
}

void
ArithmeticOpGenerator::generate(const rvsdg::SimpleNode *node,
                                RhlsToFirrtlConverter &converter) const {
  // The converter already has the logic in MlirGenSimpleNode()
  // This generator simply delegates to the converter's existing implementation
  auto debugStr = node->GetOperation().debug_string();

  if (debugStr == "IntegerAdd") {
    converter.MlirGenSimpleNode(node);
  } else if (debugStr == "IntegerSub") {
    converter.MlirGenSimpleNode(node);
  } else if (debugStr == "IntegerAnd") {
    converter.MlirGenSimpleNode(node);
  } else if (debugStr == "IntegerOr") {
    converter.MlirGenSimpleNode(node);
  } else if (debugStr == "IntegerXor") {
    converter.MlirGenSimpleNode(node);
  }
}

} // namespace generators
} // namespace rhls2firrtl
} // namespace backend
} // namespace hls
} // namespace jlm