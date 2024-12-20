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

bool
isForbiddenChar(char c);

class BaseHLS
{
public:
  std::string
  run(llvm::RvsdgModule & rm)
  {
    JLM_ASSERT(node_map.empty());
    // ensure consistent naming across runs
    create_node_names(get_hls_lambda(rm)->subregion());
    return get_text(rm);
  }

  static int
  JlmSize(const jlm::rvsdg::Type * type);

  /**
   * @return The size of a pointer in bits.
   */
  [[nodiscard]] static size_t
  GetPointerSizeInBits()
  {
    return 64;
  }

private:
  virtual std::string
  extension() = 0;

protected:
  std::unordered_map<const rvsdg::Node *, std::string> node_map;
  std::unordered_map<jlm::rvsdg::output *, std::string> output_map;

  std::string
  get_node_name(const rvsdg::Node * node);

  static std::string
  get_port_name(jlm::rvsdg::input * port);

  static std::string
  get_port_name(jlm::rvsdg::output * port);

  const llvm::lambda::node *
  get_hls_lambda(llvm::RvsdgModule & rm);

  void
  create_node_names(rvsdg::Region * r);

  virtual std::string
  get_text(llvm::RvsdgModule & rm) = 0;

  static std::string
  get_base_file_name(const llvm::RvsdgModule & rm);

  std::vector<jlm::rvsdg::RegionArgument *>
  get_mem_resps(const llvm::lambda::node * lambda)
  {
    std::vector<jlm::rvsdg::RegionArgument *> mem_resps;
    for (size_t i = 0; i < lambda->subregion()->narguments(); ++i)
    {
      auto arg = lambda->subregion()->argument(i);
      if (dynamic_cast<const jlm::hls::bundletype *>(&arg->type()))
      {
        mem_resps.push_back(lambda->subregion()->argument(i));
      }
    }
    return mem_resps;
  }

  std::vector<rvsdg::RegionResult *>
  get_mem_reqs(const llvm::lambda::node * lambda)
  {
    std::vector<rvsdg::RegionResult *> mem_resps;
    for (size_t i = 0; i < lambda->subregion()->nresults(); ++i)
    {
      if (dynamic_cast<const jlm::hls::bundletype *>(&lambda->subregion()->result(i)->type()))
      {
        mem_resps.push_back(lambda->subregion()->result(i));
      }
    }
    return mem_resps;
  }

  std::vector<jlm::rvsdg::RegionArgument *>
  get_reg_args(const llvm::lambda::node * lambda)
  {
    std::vector<jlm::rvsdg::RegionArgument *> args;
    for (size_t i = 0; i < lambda->subregion()->narguments(); ++i)
    {
      auto argtype = &lambda->subregion()->argument(i)->type();
      if (!dynamic_cast<const jlm::hls::bundletype *>(
              argtype) /*&& !dynamic_cast<const jlm::rvsdg::statetype *>(argtype)*/)
      {
        args.push_back(lambda->subregion()->argument(i));
      }
    }
    return args;
  }

  std::vector<rvsdg::RegionResult *>
  get_reg_results(const llvm::lambda::node * lambda)
  {
    std::vector<rvsdg::RegionResult *> results;
    for (size_t i = 0; i < lambda->subregion()->nresults(); ++i)
    {
      auto argtype = &lambda->subregion()->result(i)->type();
      if (!dynamic_cast<const jlm::hls::bundletype *>(
              argtype) /*&& !dynamic_cast<const jlm::rvsdg::statetype *>(argtype)*/)
      {
        results.push_back(lambda->subregion()->result(i));
      }
    }
    return results;
  }
};

}

#endif // JLM_HLS_BACKEND_RHLS2FIRRTL_BASE_HLS_HPP
