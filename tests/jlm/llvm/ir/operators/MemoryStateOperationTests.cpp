/*
 * Copyright 2024 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-types.hpp"
#include <test-operation.hpp>
#include <test-registry.hpp>

#include <jlm/llvm/ir/operators/alloca.hpp>
#include <jlm/llvm/ir/operators/Load.hpp>
#include <jlm/llvm/ir/operators/MemoryStateOperations.hpp>
#include <jlm/llvm/ir/operators/Store.hpp>
#include <jlm/llvm/ir/RvsdgModule.hpp>
#include <jlm/rvsdg/NodeNormalization.hpp>
#include <jlm/rvsdg/view.hpp>

static void
MemoryStateSplitEquality()
{
  using namespace jlm::llvm;

  // Arrange
  auto memoryStateType = MemoryStateType::Create();
  MemoryStateSplitOperation operation1(2);
  MemoryStateSplitOperation operation2(4);
  jlm::tests::TestOperation operation3({ memoryStateType }, { memoryStateType, memoryStateType });

  // Act & Assert
  assert(operation1 == operation1);
  assert(operation1 != operation2); // Number of results differ
  assert(operation1 != operation3); // Operation differs
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-MemoryStateSplitEquality",
    MemoryStateSplitEquality)

static void
MemoryStateSplitNormalizeSingleResult()
{
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;

  // Arrange
  const auto memoryStateType = MemoryStateType::Create();

  Graph rvsdg;
  auto & ix = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x");

  auto & splitNode = MemoryStateSplitOperation::CreateNode(ix, 1);

  auto & ex = jlm::rvsdg::GraphExport::Create(*splitNode.output(0), "x");

  view(&rvsdg.GetRootRegion(), stdout);

  // Act
  jlm::rvsdg::ReduceNode<MemoryStateSplitOperation>(
      MemoryStateSplitOperation::NormalizeSingleResult,
      splitNode);
  rvsdg.PruneNodes();
  view(&rvsdg.GetRootRegion(), stdout);

  // Assert
  assert(rvsdg.GetRootRegion().nnodes() == 0);
  assert(ex.origin() == &ix);
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-MemoryStateSplitNormalizeSingleResult",
    MemoryStateSplitNormalizeSingleResult)

static void
MemoryStateSplitNormalizeNestedSplits()
{
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;

  // Arrange
  const auto memoryStateType = MemoryStateType::Create();
  Graph rvsdg;
  auto & ix = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x");

  auto & splitNode0 = MemoryStateSplitOperation::CreateNode(ix, 3);
  auto & splitNode1 = MemoryStateSplitOperation::CreateNode(*splitNode0.output(0), 2);
  auto & splitNode2 = MemoryStateSplitOperation::CreateNode(*splitNode0.output(2), 2);

  auto & ex0 = jlm::rvsdg::GraphExport::Create(*splitNode1.output(0), "sn10");
  auto & ex1 = jlm::rvsdg::GraphExport::Create(*splitNode1.output(1), "sn11");
  auto & ex2 = jlm::rvsdg::GraphExport::Create(*splitNode0.output(1), "sn01");
  auto & ex3 = jlm::rvsdg::GraphExport::Create(*splitNode2.output(0), "sn20");
  auto & ex4 = jlm::rvsdg::GraphExport::Create(*splitNode2.output(1), "sn21");

  view(&rvsdg.GetRootRegion(), stdout);

  // Act
  jlm::rvsdg::ReduceNode<MemoryStateSplitOperation>(
      MemoryStateSplitOperation::NormalizeNestedSplits,
      splitNode1);
  jlm::rvsdg::ReduceNode<MemoryStateSplitOperation>(
      MemoryStateSplitOperation::NormalizeNestedSplits,
      splitNode2);
  rvsdg.PruneNodes();
  view(&rvsdg.GetRootRegion(), stdout);

  // Assert
  // We should only have MemoryStateSplit left
  assert(rvsdg.GetRootRegion().nnodes() == 1);
  auto [splitNode, splitOperation] =
      TryGetSimpleNodeAndOp<MemoryStateSplitOperation>(*ex0.origin());
  assert(splitNode && splitOperation);

  // We should have 7 outputs:
  // - 2 from splitNode1
  // - 2 from splitNode2
  // - 1 from splitNode0
  // - 1 from splitNode0 -> splitNode1
  // - 1 from splitNode0 -> splitNode2
  assert(splitNode->noutputs() == 7);
  assert(TryGetOwnerNode<SimpleNode>(*ex0.origin()) == splitNode);
  assert(TryGetOwnerNode<SimpleNode>(*ex1.origin()) == splitNode);
  assert(TryGetOwnerNode<SimpleNode>(*ex2.origin()) == splitNode);
  assert(TryGetOwnerNode<SimpleNode>(*ex3.origin()) == splitNode);
  assert(TryGetOwnerNode<SimpleNode>(*ex4.origin()) == splitNode);
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-MemoryStateSplitNormalizeNestedSplits",
    MemoryStateSplitNormalizeNestedSplits)

static void
MemoryStateSplitNormalizeSplitMerge()
{
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;

  // Arrange
  const auto memoryStateType = MemoryStateType::Create();
  Graph rvsdg;
  auto & ix0 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x");
  auto & ix1 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x");
  auto & ix2 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x");

  auto mergeResult = MemoryStateMergeOperation::Create({ &ix0, &ix1, &ix2 });
  auto & splitNode = MemoryStateSplitOperation::CreateNode(*mergeResult, 3);

  auto & ex0 = jlm::rvsdg::GraphExport::Create(*splitNode.output(0), "ex0");
  auto & ex1 = jlm::rvsdg::GraphExport::Create(*splitNode.output(1), "ex1");
  auto & ex2 = jlm::rvsdg::GraphExport::Create(*splitNode.output(2), "ex2");

  view(&rvsdg.GetRootRegion(), stdout);

  // Act
  jlm::rvsdg::ReduceNode<MemoryStateSplitOperation>(
      MemoryStateSplitOperation::NormalizeSplitMerge,
      splitNode);
  rvsdg.PruneNodes();
  view(&rvsdg.GetRootRegion(), stdout);

  // Assert
  assert(rvsdg.GetRootRegion().nnodes() == 0);
  assert(ex0.origin() == &ix0);
  assert(ex1.origin() == &ix1);
  assert(ex2.origin() == &ix2);
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-MemoryStateSplitNormalizeSplitMerge",
    MemoryStateSplitNormalizeSplitMerge)

static void
MemoryStateMergeEquality()
{
  using namespace jlm::llvm;

  // Arrange
  auto memoryStateType = MemoryStateType::Create();
  MemoryStateMergeOperation operation1(2);
  MemoryStateMergeOperation operation2(4);
  jlm::tests::TestOperation operation3({ memoryStateType, memoryStateType }, { memoryStateType });

  // Act & Assert
  assert(operation1 == operation1);
  assert(operation1 != operation2); // Number of operands differ
  assert(operation1 != operation3); // Operation differs
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-MemoryStateMergeEquality",
    MemoryStateMergeEquality)

static void
MemoryStateMergeNormalizeSingleOperand()
{
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;

  // Arrange
  const auto memoryStateType = MemoryStateType::Create();

  Graph rvsdg;
  auto & ix = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x");

  auto & mergeNode = MemoryStateMergeOperation::CreateNode({ &ix });

  auto & ex = jlm::rvsdg::GraphExport::Create(*mergeNode.output(0), "x");

  view(&rvsdg.GetRootRegion(), stdout);

  // Act
  ReduceNode<MemoryStateMergeOperation>(
      MemoryStateMergeOperation::NormalizeSingleOperand,
      mergeNode);
  rvsdg.PruneNodes();
  view(&rvsdg.GetRootRegion(), stdout);

  // Assert
  assert(rvsdg.GetRootRegion().nnodes() == 0);
  assert(ex.origin() == &ix);
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-MemoryStateMergeNormalizeSingleOperand",
    MemoryStateMergeNormalizeSingleOperand)

static void
MemoryStateMergeNormalizeDuplicateOperands()
{
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;

  // Arrange
  const auto memoryStateType = MemoryStateType::Create();

  Graph rvsdg;
  auto & ix0 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x0");
  auto & ix1 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x1");

  auto & node = MemoryStateMergeOperation::CreateNode({ &ix0, &ix0, &ix1, &ix1 });

  auto & ex = jlm::rvsdg::GraphExport::Create(*node.output(0), "x");

  view(&rvsdg.GetRootRegion(), stdout);

  // Act
  ReduceNode<MemoryStateMergeOperation>(
      MemoryStateMergeOperation::NormalizeDuplicateOperands,
      node);
  rvsdg.PruneNodes();
  view(&rvsdg.GetRootRegion(), stdout);

  // Assert
  assert(rvsdg.GetRootRegion().nnodes() == 1);
  auto [mergeNode, mergeOperation] = TryGetSimpleNodeAndOp<MemoryStateMergeOperation>(*ex.origin());
  assert(mergeNode && mergeOperation);

  assert(mergeNode->ninputs() == 2);
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-MemoryStateMergeNormalizeDuplicateOperands",
    MemoryStateMergeNormalizeDuplicateOperands)

static void
MemoryStateMergeNormalizeNestedMerges()
{
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;

  // Arrange
  const auto memoryStateType = MemoryStateType::Create();

  Graph rvsdg;
  auto & ix0 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x0");
  auto & ix1 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x1");
  auto & ix2 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x2");
  auto & ix3 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x3");
  auto & ix4 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x4");

  auto & mergeNode0 = MemoryStateMergeOperation::CreateNode({ &ix0, &ix1 });
  auto & mergeNode1 = MemoryStateMergeOperation::CreateNode({ &ix2, &ix3 });
  auto & mergeNode2 =
      MemoryStateMergeOperation::CreateNode({ mergeNode0.output(0), mergeNode1.output(0), &ix4 });

  auto & ex = jlm::rvsdg::GraphExport::Create(*mergeNode2.output(0), "x");

  view(&rvsdg.GetRootRegion(), stdout);

  // Act
  ReduceNode<MemoryStateMergeOperation>(
      MemoryStateMergeOperation::NormalizeNestedMerges,
      mergeNode2);
  rvsdg.PruneNodes();
  view(&rvsdg.GetRootRegion(), stdout);

  // Assert
  assert(rvsdg.GetRootRegion().nnodes() == 1);
  auto [mergeNode, mergeOperation] = TryGetSimpleNodeAndOp<MemoryStateMergeOperation>(*ex.origin());
  assert(mergeNode && mergeOperation);

  assert(mergeNode->ninputs() == 5);
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-MemoryStateMergeNormalizeNestedMerges",
    MemoryStateMergeNormalizeNestedMerges)

static void
MemoryStateMergeNormalizeNestedSplits()
{
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;

  // Arrange
  const auto memoryStateType = MemoryStateType::Create();

  Graph rvsdg;
  auto & ix0 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x0");
  auto & ix1 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x1");
  auto & ix2 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "x2");

  auto & splitNode0 = MemoryStateSplitOperation::CreateNode(ix0, 2);
  auto & splitNode1 = MemoryStateSplitOperation::CreateNode(ix1, 2);
  auto & mergeNode = MemoryStateMergeOperation::CreateNode({ splitNode0.output(0),
                                                             splitNode0.output(1),
                                                             splitNode1.output(0),
                                                             splitNode1.output(1),
                                                             &ix2 });

  auto & ex = jlm::rvsdg::GraphExport::Create(*mergeNode.output(0), "x");

  view(&rvsdg.GetRootRegion(), stdout);

  // Act
  ReduceNode<MemoryStateMergeOperation>(MemoryStateMergeOperation::NormalizeMergeSplit, mergeNode);
  rvsdg.PruneNodes();
  view(&rvsdg.GetRootRegion(), stdout);

  // Assert
  assert(rvsdg.GetRootRegion().nnodes() == 1);
  auto [node, mergeOperation] = TryGetSimpleNodeAndOp<MemoryStateMergeOperation>(*ex.origin());
  assert(node && mergeOperation);

  assert(node->ninputs() == 5);
  assert(node->input(0)->origin() == &ix0);
  assert(node->input(1)->origin() == &ix0);
  assert(node->input(2)->origin() == &ix1);
  assert(node->input(3)->origin() == &ix1);
  assert(node->input(4)->origin() == &ix2);
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-MemoryStateMergeNormalizeNestedSplits",
    MemoryStateMergeNormalizeNestedSplits)

static void
LambdaEntryMemStateOperatorEquality()
{
  using namespace jlm::llvm;

  // Arrange
  auto memoryStateType = MemoryStateType::Create();
  LambdaEntryMemoryStateSplitOperation operation1(2);
  LambdaEntryMemoryStateSplitOperation operation2(4);
  jlm::tests::TestOperation operation3({ memoryStateType }, { memoryStateType, memoryStateType });

  // Act & Assert
  assert(operation1 == operation1);
  assert(operation1 != operation2); // Number of results differ
  assert(operation1 != operation3); // Operation differs
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/opt/alias-analyses/MemoryStateOperationTests-LambdaEntryMemStateOperatorEquality",
    LambdaEntryMemStateOperatorEquality)

static void
LambdaExitMemStateOperatorEquality()
{
  using namespace jlm::llvm;

  // Arrange
  auto memoryStateType = MemoryStateType::Create();
  LambdaExitMemoryStateMergeOperation operation1(2);
  LambdaExitMemoryStateMergeOperation operation2(4);
  jlm::tests::TestOperation operation3({ memoryStateType, memoryStateType }, { memoryStateType });

  // Act & Assert
  assert(operation1 == operation1);
  assert(operation1 != operation2); // Number of operands differ
  assert(operation1 != operation3); // Operation differs
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-LambdaExitMemStateOperatorEquality",
    LambdaExitMemStateOperatorEquality)

static void
LambdaExitMemoryStateMergeNormalizeLoad()
{
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;

  // Arrange
  const auto bit32Type = bittype::Create(32);
  const auto memoryStateType = jlm::llvm::MemoryStateType::Create();
  const auto valueType = jlm::tests::ValueType::Create();

  Graph graph;
  auto & memState1 = jlm::tests::GraphImport::Create(graph, memoryStateType, "memState1");
  auto & memState2 = jlm::tests::GraphImport::Create(graph, memoryStateType, "memState1");
  auto & size = jlm::tests::GraphImport::Create(graph, bit32Type, "size");

  auto allocaResults = AllocaOperation::create(valueType, &size, 4);
  auto & loadNode =
      LoadNonVolatileOperation::CreateNode(*allocaResults[0], { allocaResults[1] }, valueType, 4);

  auto & lambdaExitMergeNode1 = LambdaExitMemoryStateMergeOperation::CreateNode(
      graph.GetRootRegion(),
      { loadNode.output(1), &memState1 });

  auto & lambdaExitMergeNode2 = LambdaExitMemoryStateMergeOperation::CreateNode(
      graph.GetRootRegion(),
      { &memState2, &memState1 });

  auto & x = jlm::rvsdg::GraphExport::Create(*lambdaExitMergeNode1.output(0), "x");
  auto & y = jlm::rvsdg::GraphExport::Create(*lambdaExitMergeNode2.output(0), "y");
  jlm::rvsdg::GraphExport::Create(*loadNode.output(0), "z");

  view(&graph.GetRootRegion(), stdout);

  // Act
  const auto success = jlm::rvsdg::ReduceNode<LambdaExitMemoryStateMergeOperation>(
      LambdaExitMemoryStateMergeOperation::NormalizeLoadFromAlloca,
      lambdaExitMergeNode1);
  graph.PruneNodes();

  view(&graph.GetRootRegion(), stdout);

  // Assert
  assert(success);
  assert(graph.GetRootRegion().nnodes() == 4);

  // The lambdaExitMergeNode1 should have been replaced
  const auto memStateMerge1Node = TryGetOwnerNode<Node>(*x.origin());
  assert(memStateMerge1Node != &lambdaExitMergeNode1);
  assert(memStateMerge1Node->ninputs() == 2);
  assert(memStateMerge1Node->input(0)->origin() == allocaResults[1]);
  assert(memStateMerge1Node->input(1)->origin() == &memState1);

  // The lambdaExitMergeNode2 should not have been replaced
  const auto memStateMerge2Node = TryGetOwnerNode<Node>(*y.origin());
  assert(memStateMerge2Node == &lambdaExitMergeNode2);
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-LambdaExitMemoryStateMergeNormalizeLoad",
    LambdaExitMemoryStateMergeNormalizeLoad)

static void
LambdaExitMemoryStateMergeNormalizeStore()
{
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;

  // Arrange
  const auto bit32Type = bittype::Create(32);
  const auto memoryStateType = jlm::llvm::MemoryStateType::Create();
  const auto valueType = jlm::tests::ValueType::Create();

  Graph graph;
  auto & memState1 = jlm::tests::GraphImport::Create(graph, memoryStateType, "memState1");
  auto & memState2 = jlm::tests::GraphImport::Create(graph, memoryStateType, "memState1");
  auto & size = jlm::tests::GraphImport::Create(graph, bit32Type, "size");

  auto allocaResults = AllocaOperation::create(valueType, &size, 4);
  auto & storeNode =
      StoreNonVolatileOperation::CreateNode(*allocaResults[0], size, { allocaResults[1] }, 4);

  auto & lambdaExitMergeNode1 = LambdaExitMemoryStateMergeOperation::CreateNode(
      graph.GetRootRegion(),
      { storeNode.output(0), &memState1 });

  auto & lambdaExitMergeNode2 = LambdaExitMemoryStateMergeOperation::CreateNode(
      graph.GetRootRegion(),
      { &memState2, &memState1 });

  auto & x = jlm::rvsdg::GraphExport::Create(*lambdaExitMergeNode1.output(0), "x");
  auto & y = jlm::rvsdg::GraphExport::Create(*lambdaExitMergeNode2.output(0), "y");

  view(&graph.GetRootRegion(), stdout);

  // Act
  const auto success = jlm::rvsdg::ReduceNode<LambdaExitMemoryStateMergeOperation>(
      LambdaExitMemoryStateMergeOperation::NormalizeStoreToAlloca,
      lambdaExitMergeNode1);
  graph.PruneNodes();

  view(&graph.GetRootRegion(), stdout);

  // Assert
  assert(success);
  assert(graph.GetRootRegion().nnodes() == 3);

  // The lambdaExitMergeNode1 should have been replaced
  const auto memStateMerge1Node = TryGetOwnerNode<Node>(*x.origin());
  assert(memStateMerge1Node != &lambdaExitMergeNode1);
  assert(memStateMerge1Node->ninputs() == 2);
  assert(memStateMerge1Node->input(0)->origin() == allocaResults[1]);
  assert(memStateMerge1Node->input(1)->origin() == &memState1);

  // The lambdaExitMergeNode2 should not have been replaced
  const auto memStateMerge2Node = TryGetOwnerNode<Node>(*y.origin());
  assert(memStateMerge2Node == &lambdaExitMergeNode2);
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-LambdaExitMemoryStateMergeNormalizeStore",
    LambdaExitMemoryStateMergeNormalizeStore)

static void
LambdaExitMemoryStateMergeNormalizeAlloca()
{
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;

  // Arrange
  const auto bit32Type = bittype::Create(32);
  const auto memoryStateType = jlm::llvm::MemoryStateType::Create();
  const auto valueType = jlm::tests::ValueType::Create();

  Graph graph;
  auto & memState1 = jlm::tests::GraphImport::Create(graph, memoryStateType, "memState1");
  auto & memState2 = jlm::tests::GraphImport::Create(graph, memoryStateType, "memState1");
  auto & size = jlm::tests::GraphImport::Create(graph, bit32Type, "size");

  auto allocaResults = AllocaOperation::create(valueType, &size, 4);

  auto & lambdaExitMergeNode1 = LambdaExitMemoryStateMergeOperation::CreateNode(
      graph.GetRootRegion(),
      { allocaResults[1], &memState1 });

  auto & lambdaExitMergeNode2 = LambdaExitMemoryStateMergeOperation::CreateNode(
      graph.GetRootRegion(),
      { &memState2, &memState1 });

  auto & x = jlm::rvsdg::GraphExport::Create(*lambdaExitMergeNode1.output(0), "x");
  auto & y = jlm::rvsdg::GraphExport::Create(*lambdaExitMergeNode2.output(0), "y");

  view(&graph.GetRootRegion(), stdout);

  // Act
  const auto success = jlm::rvsdg::ReduceNode<LambdaExitMemoryStateMergeOperation>(
      LambdaExitMemoryStateMergeOperation::NormalizeAlloca,
      lambdaExitMergeNode1);
  graph.PruneNodes();

  view(&graph.GetRootRegion(), stdout);

  // Assert
  assert(success);
  assert(graph.GetRootRegion().nnodes() == 3);

  // The lambdaExitMergeNode1 should have been replaced
  const auto memStateMerge1Node = TryGetOwnerNode<Node>(*x.origin());
  assert(memStateMerge1Node != &lambdaExitMergeNode1);
  assert(memStateMerge1Node->ninputs() == 2);
  const auto undefNode = TryGetOwnerNode<Node>(*memStateMerge1Node->input(0)->origin());
  assert(undefNode);
  assert(memStateMerge1Node->input(1)->origin() == &memState1);

  // The lambdaExitMergeNode2 should not have been replaced
  const auto memStateMerge2Node = TryGetOwnerNode<Node>(*y.origin());
  assert(memStateMerge2Node == &lambdaExitMergeNode2);
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-LambdaExitMemoryStateMergeNormalizeAlloca",
    LambdaExitMemoryStateMergeNormalizeAlloca)

static void
CallEntryMemStateOperatorEquality()
{
  using namespace jlm::llvm;

  // Arrange
  auto memoryStateType = MemoryStateType::Create();
  CallEntryMemoryStateMergeOperation operation1(2);
  CallEntryMemoryStateMergeOperation operation2(4);
  jlm::tests::TestOperation operation3({ memoryStateType, memoryStateType }, { memoryStateType });

  // Act & Assert
  assert(operation1 == operation1);
  assert(operation1 != operation2); // Number of operands differ
  assert(operation1 != operation3); // Operation differs
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/opt/alias-analyses/MemoryStateOperationTests-CallEntryMemStateOperatorEquality",
    CallEntryMemStateOperatorEquality)

static void
CallExitMemStateOperatorEquality()
{
  using namespace jlm::llvm;

  // Arrange
  auto memoryStateType = MemoryStateType::Create();
  CallExitMemoryStateSplitOperation operation1(2);
  CallExitMemoryStateSplitOperation operation2(4);
  jlm::tests::TestOperation operation3({ memoryStateType }, { memoryStateType, memoryStateType });

  // Act & Assert
  assert(operation1 == operation1);
  assert(operation1 != operation2); // Number of results differ
  assert(operation1 != operation3); // Operation differs
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/ir/operators/MemoryStateOperationTests-CallExitMemStateOperatorEquality",
    CallExitMemStateOperatorEquality)

static void
CallExitMemoryStateSplit_NormalizeLambdaExitMerge()
{
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;

  // Arrange
  const auto memoryStateType = MemoryStateType::Create();

  Graph rvsdg;
  auto & i0 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "i0");
  auto & i1 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "i1");
  auto & i2 = jlm::tests::GraphImport::Create(rvsdg, memoryStateType, "i2");

  auto & callEntryMergeNode =
      LambdaExitMemoryStateMergeOperation::CreateNode(rvsdg.GetRootRegion(), { &i0, &i1, &i2 });

  auto & lambdaEntrySplitNode =
      CallExitMemoryStateSplitOperation::CreateNode(*callEntryMergeNode.output(0), 3);

  auto & x0 = jlm::rvsdg::GraphExport::Create(*lambdaEntrySplitNode.output(0), "x0");
  auto & x1 = jlm::rvsdg::GraphExport::Create(*lambdaEntrySplitNode.output(1), "x1");
  auto & x2 = jlm::rvsdg::GraphExport::Create(*lambdaEntrySplitNode.output(2), "x2");

  view(&rvsdg.GetRootRegion(), stdout);

  // Act
  const auto success = jlm::rvsdg::ReduceNode<CallExitMemoryStateSplitOperation>(
      CallExitMemoryStateSplitOperation::NormalizeLambdaExitMemoryStateMerge,
      lambdaEntrySplitNode);
  rvsdg.PruneNodes();

  view(&rvsdg.GetRootRegion(), stdout);

  // Assert
  assert(success);
  assert(rvsdg.GetRootRegion().nnodes() == 0);

  assert(x0.origin() == &i0);
  assert(x1.origin() == &i1);
  assert(x2.origin() == &i2);
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/opt/alias-analyses/"
    "MemoryStateOperationTests-CallExitMemoryStateSplit_NormalizeLambdaExitMerge",
    CallExitMemoryStateSplit_NormalizeLambdaExitMerge)
