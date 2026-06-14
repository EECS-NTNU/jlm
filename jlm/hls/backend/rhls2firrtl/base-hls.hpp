/*
 * Copyright 2021 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_HLS_BACKEND_RHLS2FIRRTL_BASE_HLS_HPP
#define JLM_HLS_BACKEND_RHLS2FIRRTL_BASE_HLS_HPP

#include <jlm/hls/ir/hls.hpp>
#include <jlm/llvm/ir/operators/lambda.hpp>
#include <jlm/llvm/ir/operators/operators.hpp>
#include <jlm/llvm/ir/RvsdgModule.hpp>

#include <fstream>

namespace jlm::hls
{

/**
 * \brief Checks if a character is forbidden in HLS node names.
 */
bool
isForbiddenChar(char c);

/**
 * \brief Abstract base class for HLS backends that generate text output from R-HLS modules.
 *
 * BaseHLS provides common functionality for HLS backends including:
 * - Node and port naming/mapping utilities
 * - Memory response/request extraction helpers (get_mem_resps, get_mem_reqs)
 * - Register argument/result extraction helpers (get_reg_args, get_reg_results)
 */
class BaseHLS
{
public:
  virtual ~BaseHLS();

  /**
   * \brief Runs the HLS backend on an LLVM RVSDG module.
   *
   * Creates consistent node names and returns a text representation of the output.
   */
  std::string
  run(llvm::LlvmRvsdgModule & rm)
  {
    JLM_ASSERT(node_map.empty());
    // ensure consistent naming across runs
    create_node_names(get_hls_lambda(rm)->subregion());
    return GetText(rm);
  }

  /**
   * \brief Returns the size of a type in bits.
   */
  static int
  JlmSize(const jlm::rvsdg::Type * type);

private:
  virtual std::string
  extension() = 0;

protected:
  std::unordered_map<const rvsdg::Node *, std::string> node_map;
  std::unordered_map<jlm::rvsdg::Output *, std::string> output_map;

  std::string
  get_node_name(const rvsdg::Node * node);

  static std::string
  get_port_name(jlm::rvsdg::Input * port);

  static std::string
  get_port_name(jlm::rvsdg::Output * port);

  const rvsdg::LambdaNode *
  get_hls_lambda(llvm::LlvmRvsdgModule & rm);

  void
  create_node_names(rvsdg::Region * r);

  virtual std::string
  GetText(llvm::LlvmRvsdgModule & rm) = 0;

  static std::string
  get_base_file_name(const llvm::LlvmRvsdgModule & rm);

  /**
   * \brief Extracts memory response arguments from a kernel lambda node.
   *
   * Memory responses provide multiple values within a single execution of the region.
   * @param lambda The lambda node holding the HLS kernel.
   * @return Arguments representing memory responses.
   */
  std::vector<rvsdg::RegionArgument *>
  get_mem_resps(const rvsdg::LambdaNode & lambda)
  {
    std::vector<rvsdg::RegionArgument *> mem_resps;
    for (auto arg : lambda.subregion()->Arguments())
    {
      if (rvsdg::is<BundleType>(arg->Type()))
        mem_resps.push_back(arg);
    }
    return mem_resps;
  }

  /**
   * \brief Extracts memory request results from a kernel lambda node.
   *
   * Memory requests take multiple values within a single execution of the region.
   * @param lambda The lambda node holding the HLS kernel.
   * @return Results representing memory requests.
   */
  std::vector<rvsdg::RegionResult *>
  get_mem_reqs(const rvsdg::LambdaNode & lambda)
  {
    std::vector<rvsdg::RegionResult *> mem_resps;
    for (auto result : lambda.subregion()->Results())
    {
      if (rvsdg::is<BundleType>(result->Type()))
        mem_resps.push_back(result);
    }
    return mem_resps;
  }

  /**
   * \brief Extracts register arguments from a kernel lambda node.
   *
   * Register arguments represent kernel inputs that are not memory responses,
   * including kernel arguments, state types, and context variables.
   * @param lambda The lambda node holding the HLS kernel.
   * @return Arguments representing kernel inputs (non-bundle type).
   */
  std::vector<rvsdg::RegionArgument *>
  get_reg_args(const rvsdg::LambdaNode & lambda)
  {
    std::vector<rvsdg::RegionArgument *> args;
    for (auto argument : lambda.subregion()->Arguments())
    {
      if (!rvsdg::is<BundleType>(argument->Type()))
        args.push_back(argument);
    }
    return args;
  }

  /**
   * \brief Extracts register results from a kernel lambda node.
   *
   * Register results represent actual execution outputs, excluding memory requests.
   * @param lambda The lambda node holding the HLS kernel.
   * @return Results representing kernel outputs (non-bundle type).
   */
  std::vector<rvsdg::RegionResult *>
  get_reg_results(const rvsdg::LambdaNode & lambda)
  {
    std::vector<rvsdg::RegionResult *> results;
    for (auto result : lambda.subregion()->Results())
    {
      if (!rvsdg::is<BundleType>(result->Type()))
        results.push_back(result);
    }
    return results;
  }
};

}

#endif // JLM_HLS_BACKEND_RHLS2FIRRTL_BASE_HLS_HPP