/*
 * Copyright 2021 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_BACKEND_HLS_RVSDG2RHLS_MEM_CONV_HPP
#define JLM_BACKEND_HLS_RVSDG2RHLS_MEM_CONV_HPP

#include <jlm/llvm/ir/operators/IntegerOperations.hpp>
#include <jlm/llvm/ir/operators/lambda.hpp>
#include <jlm/rvsdg/Transformation.hpp>

namespace jlm::hls
{

/**
 * \brief Tracks memory operations traced from pointer arguments.
 *
 * Contains vectors of load, store, and decouple nodes that were found during pointer tracing.
 */
struct TracedPointerNodes
{
  /**
   * \brief Returns true if no memory operations were traced.
   */
  [[nodiscard]] bool
  isEmpty() const noexcept
  {
    return loadNodes.empty() && storeNodes.empty() && decoupleNodes.empty();
  }

  std::vector<rvsdg::Node *> loadNodes{};
  std::vector<rvsdg::Node *> storeNodes{};
  std::vector<rvsdg::Node *> decoupleNodes{};
};

/**
 * \brief Traces all pointer arguments of a lambda node and finds all memory operations.
 *
 * This function traces through the RVSDG to identify all memory operations (load, store, decouple)
 * that are reachable from the lambda's pointer-typed arguments. Note that outputs of load
 * operations themselves are not traced further - only the memory access chain is followed.
 *
 * @param lambda The lambda node for which to trace all pointer arguments.
 * @return A vector where each element contains all memory operations traced from one pointer.
 */
std::vector<TracedPointerNodes>
TracePointerArguments(const rvsdg::LambdaNode * lambda);

/**
 * \brief Finds a decouple response node that matches the given request constant.
 *
 * @param lambda The lambda node containing the memory operations.
 * @param request_constant The integer constant identifying the memory request.
 * @return The corresponding decouple response node, or nullptr if not found.
 */
jlm::rvsdg::SimpleNode *
find_decouple_response(
    const jlm::rvsdg::LambdaNode * lambda,
    const jlm::llvm::IntegerConstantOperation * request_constant);

/**
 * \brief Converts memory operations to explicit memory ports in the HLS representation.
 *
 * This transformation replaces implicit memory state edges with explicit request/response
 * bundles using BundleType. It's required for generating proper hardware interfaces (AXI, etc.)
 * in the FIRRTL backend.
 */
class MemoryConverter final : public rvsdg::Transformation
{
public:
  ~MemoryConverter() noexcept override;

  MemoryConverter();

  MemoryConverter(const MemoryConverter &) = delete;

  MemoryConverter &
  operator=(const MemoryConverter &) = delete;

  void
  Run(rvsdg::RvsdgModule & rvsdgModule, util::StatisticsCollector & statisticsCollector) override;

  /**
   * \brief Creates and runs a memory conversion transformation.
   */
  static void
  CreateAndRun(rvsdg::RvsdgModule & rvsdgModule, util::StatisticsCollector & statisticsCollector)
  {
    MemoryConverter memoryConverter;
    memoryConverter.Run(rvsdgModule, statisticsCollector);
  }
};

} // namespace jlm::hls

#endif // JLM_BACKEND_HLS_RVSDG2RHLS_MEM_CONV_HPP