/*
 * Copyright 2021 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_HLS_BACKEND_RVSDG2RHLS_RVSDG2RHLS_HPP
#define JLM_HLS_BACKEND_RVSDG2RHLS_RVSDG2RHLS_HPP

#include <jlm/llvm/ir/operators/IntegerOperations.hpp>
#include <jlm/llvm/ir/operators/operators.hpp>
#include <jlm/llvm/ir/RvsdgModule.hpp>
#include <jlm/rvsdg/node.hpp>
#include <jlm/rvsdg/Transformation.hpp>
#include <jlm/util/Statistics.hpp>

namespace jlm::hls
{

/**
 * \brief Checks if a node is a constant operation.
 *
 * \param node The node to check.
 * \return true if the node is an IntegerConstantOperation, UndefValueOperation,
 *         ConstantFP, or ControlConstantOperation; false otherwise.
 */
static inline bool
is_constant(const rvsdg::Node * node)
{
  return jlm::rvsdg::is<llvm::IntegerConstantOperation>(node)
      || jlm::rvsdg::is<llvm::UndefValueOperation>(node) || jlm::rvsdg::is<llvm::ConstantFP>(node)
      || jlm::rvsdg::is<rvsdg::ControlConstantOperation>(node);
}

/**
 * \brief Creates a transformation sequence for RVSDG to R-HLS conversion.
 *
 * \param dotWriter The dot writer used for graph visualization.
 * \param dumpRvsdgGraphs Whether to dump intermediate graphs during conversion.
 * \return A unique pointer to the transformation sequence.
 */
std::unique_ptr<rvsdg::TransformationSequence>
createTransformationSequence(rvsdg::DotWriter & dotWriter, bool dumpRvsdgGraphs);

/**
 * \brief Converts an LLVM RVSDG module to the reference R-HLS format (ref).
 *
 * \param rm The LLVM RVSDG module to convert.
 * \param function_name The name used for output files.
 */
void
rvsdg2ref(llvm::LlvmRvsdgModule & rm, const util::FilePath & function_name);

/**
 * \brief Dumps the R-HLS representation to a reference file.
 *
 * \param rhls The R-HLS module to dump.
 * \param function_name The name used for output files.
 */
void
dump_ref(llvm::LlvmRvsdgModule & rhls, const util::FilePath & function_name);

/**
 * \brief Traces through call operations to find the original input source.
 *
 * \param input The input to trace backwards from.
 * \return The original output that feeds into this input, or nullptr if not found.
 */
const jlm::rvsdg::Output *
trace_call(jlm::rvsdg::Input * input);

/**
 * \brief Splits an HLS function from a module into its own module.
 *
 * \param rm The source LLVM RVSDG module.
 * \param function_name The name of the function to extract.
 * \return A new module containing only the specified HLS function.
 */
std::unique_ptr<llvm::LlvmRvsdgModule>
split_hls_function(llvm::LlvmRvsdgModule & rm, const std::string & function_name);

}

#endif // JLM_HLS_BACKEND_RVSDG2RHLS_RVSDG2RHLS_HPP