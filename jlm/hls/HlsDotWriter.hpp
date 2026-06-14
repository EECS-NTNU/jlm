/*
 * Copyright 2025 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_HLS_HLSDOTWRITER_HPP
#define JLM_HLS_HLSDOTWRITER_HPP

#include <jlm/llvm/DotWriter.hpp>

namespace jlm::hls
{

/**
 * \brief Custom dot writer for HLS-specific graphs.
 *
 * Extends the base LLVM dot writer to provide custom annotation of type graph nodes
 * in the HLS backend visualization.
 */
class HlsDotWriter final : public llvm::LlvmDotWriter
{
public:
  ~HlsDotWriter() noexcept override;

protected:
  /**
   * \brief Annotates a type graph node with HLS-specific information.
   *
   * \param type The type being visualized.
   * \param node The dot graph node to annotate.
   */
  void
  AnnotateTypeGraphNode(const rvsdg::Type & type, util::graph::Node & node) override;
};

}

#endif