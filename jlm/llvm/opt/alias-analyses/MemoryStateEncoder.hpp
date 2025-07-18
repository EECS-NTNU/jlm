/*
 * Copyright 2021 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_LLVM_OPT_ALIAS_ANALYSES_MEMORYSTATEENCODER_HPP
#define JLM_LLVM_OPT_ALIAS_ANALYSES_MEMORYSTATEENCODER_HPP

#include <memory>
#include <vector>

namespace jlm::rvsdg
{
class GammaNode;
class LambdaNode;
class Output;
class PhiNode;
class Region;
class RvsdgModule;
class SimpleNode;
class StructuralNode;
class ThetaNode;
}

namespace jlm::util
{
class StatisticsCollector;
}

namespace jlm::llvm
{

class DeltaNode;

namespace aa
{

class ModRefSummary;

/** \brief Memory State Encoder
 *
 * A memory state encoder encodes a points-to graph in the RVSDG. The basic idea is that there
 * exists a one-to-one correspondence between memory nodes in the points-to graph and memory states
 * in the RVSDG, i.e., for each memory node in the points-to graph, there exists a memory state edge
 * in the RVSDG. A memory state encoder routes these state edges through the RVSDG's structural
 * nodes and ensures that simple nodes operating on a memory location represented by a corresponding
 * memory node in the points-to graph are sequentialized with the respective memory state edge. For
 * example, a store node that modifies a global variable needs to have the respective state edge
 * that corresponds to its memory location routed through it, i.e., the store node is sequentialized
 * by this state edge. Such an encoding ensures that the ordering of side-effecting operations
 * touching on the same memory locations is preserved, while rendering operations independent that
 * are not operating on the same memory locations.
 */
class MemoryStateEncoder final
{
public:
  class Context;

  ~MemoryStateEncoder() noexcept;

  MemoryStateEncoder();

  MemoryStateEncoder(const MemoryStateEncoder &) = delete;

  MemoryStateEncoder(MemoryStateEncoder &&) = delete;

  MemoryStateEncoder &
  operator=(const MemoryStateEncoder &) = delete;

  MemoryStateEncoder &
  operator=(MemoryStateEncoder &&) = delete;

  void
  Encode(
      rvsdg::RvsdgModule & rvsdgModule,
      const ModRefSummary & modRefSummary,
      util::StatisticsCollector & statisticsCollector);

private:
  void
  EncodeRegion(rvsdg::Region & region);

  void
  EncodeStructuralNode(rvsdg::StructuralNode & structuralNode);

  void
  EncodeSimpleNode(const rvsdg::SimpleNode & simpleNode);

  void
  EncodeAlloca(const rvsdg::SimpleNode & allocaNode);

  void
  EncodeMalloc(const rvsdg::SimpleNode & mallocNode);

  void
  EncodeLoad(const rvsdg::SimpleNode & node);

  void
  EncodeStore(const rvsdg::SimpleNode & node);

  void
  EncodeFree(const rvsdg::SimpleNode & freeNode);

  void
  EncodeCall(const rvsdg::SimpleNode & callNode);

  void
  EncodeCallEntry(const rvsdg::SimpleNode & callNode);

  void
  EncodeCallExit(const rvsdg::SimpleNode & callNode);

  void
  EncodeMemcpy(const rvsdg::SimpleNode & memcpyNode);

  void
  EncodeLambda(const rvsdg::LambdaNode & lambda);

  void
  EncodeLambdaEntry(const rvsdg::LambdaNode & lambdaNode);

  void
  EncodeLambdaExit(const rvsdg::LambdaNode & lambdaNode);

  void
  EncodePhi(const rvsdg::PhiNode & phiNode);

  void
  EncodeDelta(const DeltaNode & deltaNode);

  void
  EncodeGamma(rvsdg::GammaNode & gammaNode);

  void
  EncodeGammaEntry(rvsdg::GammaNode & gammaNode);

  void
  EncodeGammaExit(rvsdg::GammaNode & gammaNode);

  void
  EncodeTheta(rvsdg::ThetaNode & thetaNode);

  std::vector<rvsdg::Output *>
  EncodeThetaEntry(rvsdg::ThetaNode & thetaNode);

  void
  EncodeThetaExit(
      rvsdg::ThetaNode & thetaNode,
      const std::vector<rvsdg::Output *> & thetaStateOutputs);

  /**
   * Replace \p loadNode with a new copy that takes the provided \p memoryStates. All users of the
   * outputs of \p loadNode are redirected to the respective outputs of the newly created copy.
   *
   * @param node A LoadNode.
   * @param memoryStates The memory states the new LoadNode should consume.
   *
   * @return The newly created LoadNode.
   */
  [[nodiscard]] static rvsdg::SimpleNode &
  ReplaceLoadNode(
      const rvsdg::SimpleNode & node,
      const std::vector<rvsdg::Output *> & memoryStates);

  /**
   * Replace \p storeNode with a new copy that takes the provided \p memoryStates. All users of the
   * outputs of \p storeNode are redirected to the respective outputs of the newly created copy.
   *
   * @param node A StoreNode.
   * @param memoryStates The memory states the new StoreNode should consume.
   *
   * @return The newly created StoreNode.
   */
  [[nodiscard]] static rvsdg::SimpleNode &
  ReplaceStoreNode(
      const rvsdg::SimpleNode & node,
      const std::vector<rvsdg::Output *> & memoryStates);

  /**
   * Replace \p memcpyNode with a new copy that takes the provided \p memoryStates. All users of
   * the outputs of \p memcpyNode are redirected to the respective outputs of the newly created
   * copy.
   *
   * @param memcpyNode A rvsdg::SimpleNode representing a MemCpyOperation.
   * @param memoryStates The memory states the new memcpy node should consume.
   *
   * @return A vector with the memory states of the newly created copy.
   */
  [[nodiscard]] static std::vector<rvsdg::Output *>
  ReplaceMemcpyNode(
      const rvsdg::SimpleNode & memcpyNode,
      const std::vector<rvsdg::Output *> & memoryStates);

  /**
   * Determines whether \p simpleNode should be handled by the MemoryStateEncoder.
   *
   * @param simpleNode A SimpleNode.
   * @return True, if \p simpleNode should be handled, otherwise false.
   */
  [[nodiscard]] static bool
  ShouldHandle(const rvsdg::SimpleNode & simpleNode) noexcept;

  std::unique_ptr<Context> Context_;
};

}
}

#endif // JLM_LLVM_OPT_ALIAS_ANALYSES_MEMORYSTATEENCODER_HPP
