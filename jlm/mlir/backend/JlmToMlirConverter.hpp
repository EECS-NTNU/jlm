/*
 * Copyright 2023 Magnus Sjalander <work@sjalander.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_MLIR_BACKEND_JLMTOMLIRCONVERTER_HPP
#define JLM_MLIR_BACKEND_JLMTOMLIRCONVERTER_HPP

// JLM
#include <jlm/llvm/ir/operators/delta.hpp>
#include <jlm/llvm/ir/operators/lambda.hpp>
#include <jlm/llvm/ir/operators/operators.hpp>
#include <jlm/llvm/ir/RvsdgModule.hpp>
#include <jlm/rvsdg/bitstring/arithmetic.hpp>
#include <jlm/rvsdg/gamma.hpp>
#include <jlm/rvsdg/theta.hpp>

// MLIR RVSDG dialects
#include <JLM/JLMDialect.h>
#include <RVSDG/RVSDGDialect.h>
#include <RVSDG/RVSDGPasses.h>

// MLIR JLM dialects
#include <JLM/JLMOps.h>

// MLIR generic dialects
#include <mlir/Dialect/Arith/IR/Arith.h>
#include <mlir/Dialect/LLVMIR/LLVMDialect.h>

namespace jlm::mlir
{

class JlmToMlirConverter final
{
public:
  JlmToMlirConverter()
      : Context_(std::make_unique<::mlir::MLIRContext>())
  {
    Context_->getOrLoadDialect<::mlir::rvsdg::RVSDGDialect>();
    Context_->getOrLoadDialect<::mlir::jlm::JLMDialect>();
    Context_->getOrLoadDialect<::mlir::arith::ArithDialect>();
    Context_->getOrLoadDialect<::mlir::LLVM::LLVMDialect>();
    Builder_ = std::make_unique<::mlir::OpBuilder>(Context_.get());
  }

  JlmToMlirConverter(const JlmToMlirConverter &) = delete;

  JlmToMlirConverter(JlmToMlirConverter &&) = delete;

  JlmToMlirConverter &
  operator=(const JlmToMlirConverter &) = delete;

  JlmToMlirConverter &
  operator=(JlmToMlirConverter &&) = delete;

  /**
   * Prints MLIR RVSDG to a file.
   * \param omega The MLIR RVSDG Omega node to be printed.
   * \param filePath The path to the file to print the MLIR to.
   */
  static void
  Print(::mlir::rvsdg::OmegaNode & omega, const util::filepath & filePath);

  /**
   * Converts an RVSDG module to MLIR RVSDG.
   * \param rvsdgModule The RVSDG module to be converted.
   * \return An MLIR RVSDG OmegaNode containing the whole graph of the rvsdgModule. It is
   * the responsibility of the caller to call ->destroy() on the returned omega, once it is no
   * longer needed.
   */
  ::mlir::rvsdg::OmegaNode
  ConvertModule(const llvm::RvsdgModule & rvsdgModule);

private:
  /**
   * Converts an omega and all nodes in its (sub)region(s) to an MLIR RVSDG OmegaNode.
   * \param graph The root RVSDG graph.
   * \return An MLIR RVSDG OmegaNode.
   */
  ::mlir::rvsdg::OmegaNode
  ConvertOmega(const rvsdg::Graph & graph);

  /**
   * Converts all nodes in an RVSDG region. Conversion of structural nodes cause their regions to
   * also be converted.
   * \param region The RVSDG region to be converted
   * \param block The MLIR RVSDG block that corresponds to this RVSDG region, and to which
   *              converted nodes are insterted.
   * \return A list of outputs of the converted region/block.
   */
  ::llvm::SmallVector<::mlir::Value>
  ConvertRegion(rvsdg::Region & region, ::mlir::Block & block);

  /**
   * Retreive the previously converted MLIR values from the map of operations
   * \param node The RVSDG node to get the inputs for.
   * \param operationsMap A map of RVSDG nodes to their corresponding MLIR operations.
   * \param block The MLIR block to get argument type inputs from.
   * \return The vector of inputs to the node.
   */
  static ::llvm::SmallVector<::mlir::Value>
  GetConvertedInputs(
      const rvsdg::Node & node,
      const std::unordered_map<rvsdg::Node *, ::mlir::Operation *> & operationsMap,
      ::mlir::Block & block);

  /**
   * Converts an RVSDG node to an MLIR RVSDG operation.
   * \param node The RVSDG node to be converted
   * \param block The MLIR RVSDG block to insert the converted node.
   * \param inputs The inputs to the node.
   * \return The converted MLIR RVSDG operation.
   */
  ::mlir::Operation *
  ConvertNode(
      const rvsdg::Node & node,
      ::mlir::Block & block,
      const ::llvm::SmallVector<::mlir::Value> & inputs);

  /**
   * Converts a floating point binary operation to an MLIR operation.
   * \param op The jlm::llvm::fpbin_op to be converted
   * \param inputs The inputs to the jlm::llvm::fpbin_op.
   * \return The converted MLIR operation.
   */
  ::mlir::Operation *
  ConvertFpBinaryNode(const jlm::llvm::fpbin_op & op, ::llvm::SmallVector<::mlir::Value> inputs);

  /**
   * Converts an RVSDG binary_op to an MLIR RVSDG operation.
   * \param bitOp The RVSDG bitbinary_op to be converted
   * \param inputs The inputs to the bitbinary_op.
   * \return The converted MLIR RVSDG operation.
   */
  ::mlir::Operation *
  ConvertBitBinaryNode(
      const rvsdg::SimpleOperation & bitOp,
      ::llvm::SmallVector<::mlir::Value> inputs);

  /**
   * Converts an RVSDG bitcompare_op to an MLIR RVSDG operation.
   * \param bitOp The RVSDG bitcompare_op to be converted
   * \param inputs The inputs to the bitcompare_op.
   * \return The converted MLIR RVSDG operation.
   */
  ::mlir::Operation *
  BitCompareNode(const rvsdg::SimpleOperation & bitOp, ::llvm::SmallVector<::mlir::Value> inputs);

  /**
   * Converts an RVSDG SimpleNode to an MLIR RVSDG operation.
   * \param node The RVSDG node to be converted
   * \param block The MLIR RVSDG block to insert the converted node.
   * \param inputs The inputs to the SimpleNode.
   * \return The converted MLIR RVSDG operation.
   */
  ::mlir::Operation *
  ConvertSimpleNode(
      const rvsdg::SimpleNode & node,
      ::mlir::Block & block,
      const ::llvm::SmallVector<::mlir::Value> & inputs);

  /**
   * Converts an RVSDG lambda node to an MLIR RVSDG LambdaNode.
   * \param node The RVSDG lambda node to be converted
   * \param block The MLIR RVSDG block to insert the lambda node.
   * \param inputs The inputs to the lambda::node.
   * \return The converted MLIR RVSDG LambdaNode.
   */
  ::mlir::Operation *
  ConvertLambda(
      const rvsdg::LambdaNode & node,
      ::mlir::Block & block,
      const ::llvm::SmallVector<::mlir::Value> & inputs);

  /**
   * Converts an RVSDG gamma node to an MLIR RVSDG GammaNode.
   * \param gammaNode The RVSDG gamma node to be converted
   * \param block The MLIR RVSDG block to insert the gamma node.
   * \param inputs The inputs to the gamma node.
   * \return The converted MLIR RVSDG GammaNode.
   */
  ::mlir::Operation *
  ConvertGamma(
      const rvsdg::GammaNode & gammaNode,
      ::mlir::Block & block,
      const ::llvm::SmallVector<::mlir::Value> & inputs);

  ::mlir::Operation *
  ConvertTheta(
      const rvsdg::ThetaNode & thetaNode,
      ::mlir::Block & block,
      const ::llvm::SmallVector<::mlir::Value> & inputs);

  /**
   * Converts an RVSDG delta node to an MLIR RVSDG DeltaNode.
   * \param node The RVSDG delta node to be converted
   * \param block The MLIR RVSDG block to insert the delta node.
   * \param inputs The inputs to the delta::node.
   * \return The converted MLIR RVSDG DeltaNode.
   */
  ::mlir::Operation *
  ConvertDelta(
      const llvm::delta::node & node,
      ::mlir::Block & block,
      const ::llvm::SmallVector<::mlir::Value> & inputs);

  /**
   * Converts an RVSDG floating point size to an MLIR floating point type.
   * \param size The jlm::llvm::fpsize to be converted.
   * \result The corresponding mlir::FloatType.
   */
  ::mlir::FloatType
  ConvertFPType(const llvm::fpsize size);

  /**
   * Converts an JLM function type to an MLIR LLVM function type.
   * \param functionType The JLM function type to be converted.
   * \result The corresponding MLIR LLVM function type.
   */
  ::mlir::FunctionType
  ConvertFunctionType(const jlm::rvsdg::FunctionType & functionType);

  /**
   * Converts an RVSDG type to an MLIR RVSDG type.
   * \param type The RVSDG type to be converted.
   * \result The corresponding MLIR RVSDG type.
   */
  ::mlir::Type
  ConvertType(const rvsdg::Type & type);

  std::unique_ptr<::mlir::OpBuilder> Builder_;
  std::unique_ptr<::mlir::MLIRContext> Context_;
};

} // namespace jlm::mlir

#endif // JLM_MLIR_BACKEND_JLMTOMLIRCONVERTER_HPP
