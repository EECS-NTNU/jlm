/*
 * Copyright 2024 Magnus Sjalander <work@sjalander.com>
 * See COPYING for terms of redistribution.
 */

#include "test-registry.hpp"

#include <jlm/hls/backend/rvsdg2rhls/mem-conv.hpp>
#include <jlm/hls/backend/rvsdg2rhls/mem-queue.hpp>
#include <jlm/hls/backend/rvsdg2rhls/mem-sep.hpp>
#include <jlm/hls/backend/rvsdg2rhls/rhls-dne.hpp>
#include <jlm/hls/backend/rvsdg2rhls/ThetaConversion.hpp>
#include <jlm/hls/ir/hls.hpp>
#include <jlm/llvm/ir/operators.hpp>
#include <jlm/rvsdg/view.hpp>
#include <jlm/util/Statistics.hpp>

static int
TestTraceArgument()
{
  using namespace jlm::llvm;
  using namespace jlm::hls;

  auto rvsdgModule = RvsdgModule::Create(jlm::util::filepath(""), "", "");

  // Setup the function
  std::cout << "Function Setup" << std::endl;
  auto functionType = jlm::rvsdg::FunctionType::Create(
      { jlm::llvm::PointerType::Create(),
        jlm::llvm::PointerType::Create(),
        jlm::rvsdg::bittype::Create(32),
        MemoryStateType::Create() },
      { MemoryStateType::Create() });

  auto lambda = jlm::rvsdg::LambdaNode::Create(
      rvsdgModule->Rvsdg().GetRootRegion(),
      LlvmLambdaOperation::Create(functionType, "test", linkage::external_linkage));

  // Load followed by store
  auto loadAddress = lambda->GetFunctionArguments()[0];
  auto memoryStateArgument = lambda->GetFunctionArguments()[3];
  auto loadOutput = LoadNonVolatileOperation::Create(
      loadAddress,
      { memoryStateArgument },
      jlm::llvm::PointerType::Create(),
      32);

  auto storeAddress = lambda->GetFunctionArguments()[1];
  auto storeData = lambda->GetFunctionArguments()[2];
  auto storeOutput = StoreNonVolatileNode::Create(storeAddress, storeData, { loadOutput[1] }, 32);

  auto lambdaOutput = lambda->finalize({ storeOutput[0] });
  jlm::llvm::GraphExport::Create(*lambdaOutput, "f");

  // Act
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);
  port_load_store_decouple portNodes;
  TracePointerArguments(lambda, portNodes);
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);

  // Assert
  assert(portNodes.size() == 2);                 // 2 pointer arguments
  assert(std::get<0>(portNodes[0]).size() == 1); // 1 load for the first pointer
  assert(std::get<1>(portNodes[0]).size() == 0); // 0 store for the first pointer
  assert(std::get<2>(portNodes[0]).size() == 0); // 0 decouple for the first pointer
  assert(std::get<0>(portNodes[1]).size() == 0); // 0 load for the first pointer
  assert(std::get<1>(portNodes[1]).size() == 1); // 1 store for the second pointer
  assert(std::get<2>(portNodes[1]).size() == 0); // 0 load for the first pointer

  return 0;
}
JLM_UNIT_TEST_REGISTER(
    "jlm/hls/backend/rvsdg2rhls/MemoryConverterTests-TraceArgument",
    TestTraceArgument)

static int
TestLoad()
{
  using namespace jlm::llvm;
  using namespace jlm::hls;

  auto rvsdgModule = RvsdgModule::Create(jlm::util::filepath(""), "", "");

  // Setup the function
  std::cout << "Function Setup" << std::endl;
  auto functionType = jlm::rvsdg::FunctionType::Create(
      { jlm::llvm::PointerType::Create(), MemoryStateType::Create() },
      { jlm::rvsdg::bittype::Create(32), MemoryStateType::Create() });

  auto lambda = jlm::rvsdg::LambdaNode::Create(
      rvsdgModule->Rvsdg().GetRootRegion(),
      LlvmLambdaOperation::Create(functionType, "test", linkage::external_linkage));

  // Single load
  auto loadAddress = lambda->GetFunctionArguments()[0];
  auto memoryStateArgument = lambda->GetFunctionArguments()[1];
  auto loadOutput = LoadNonVolatileOperation::Create(
      loadAddress,
      { memoryStateArgument },
      jlm::rvsdg::bittype::Create(32),
      32);

  auto lambdaOutput = lambda->finalize({ loadOutput[0], loadOutput[1] });
  jlm::llvm::GraphExport::Create(*lambdaOutput, "f");

  // Act
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);
  MemoryConverter(*rvsdgModule);
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);

  // Memory Converter replaces the lambda so we start from the root of the graph
  auto region = &rvsdgModule->Rvsdg().GetRootRegion();
  assert(region->nnodes() == 1);
  lambda = jlm::util::AssertedCast<jlm::rvsdg::LambdaNode>(region->Nodes().begin().ptr());

  // Assert
  auto lambdaRegion = lambda->subregion();
  assert(lambdaRegion->nnodes() == 3);
  assert(lambdaRegion->narguments() == 3);
  assert(lambdaRegion->nresults() == 3);

  // Memory state
  assert(is<MemoryStateType>(lambdaRegion->result(1)->origin()->type()));

  // Load Address
  auto loadNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(lambdaRegion->result(0)->origin())->node();
  assert(is<load_op>(loadNode));

  // Load Data
  loadNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(lambdaRegion->result(1)->origin())->node();
  assert(is<load_op>(loadNode));

  // Request Node
  auto requestNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(lambdaRegion->result(2)->origin())->node();
  assert(is<const mem_req_op>(requestNode));

  // Response Node
  auto responseNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(loadNode->input(2)->origin())->node();
  assert(is<mem_resp_op>(responseNode));

  // Response source
  auto responseSource = responseNode->input(0)->origin();
  auto regionArgument = jlm::util::AssertedCast<jlm::rvsdg::RegionArgument>(responseSource);
  assert(regionArgument->index() == 2);

  return 0;
}
JLM_UNIT_TEST_REGISTER("jlm/hls/backend/rvsdg2rhls/MemoryConverterTests-Load", TestLoad)

static int
TestStore()
{
  using namespace jlm::llvm;
  using namespace jlm::hls;

  auto rvsdgModule = RvsdgModule::Create(jlm::util::filepath(""), "", "");

  // Setup the function
  std::cout << "Function Setup" << std::endl;
  auto functionType = jlm::rvsdg::FunctionType::Create(
      { jlm::llvm::PointerType::Create(),
        jlm::rvsdg::bittype::Create(32),
        MemoryStateType::Create() },
      { MemoryStateType::Create() });

  auto lambda = jlm::rvsdg::LambdaNode::Create(
      rvsdgModule->Rvsdg().GetRootRegion(),
      LlvmLambdaOperation::Create(functionType, "test", linkage::external_linkage));

  // Single load
  auto storeAddress = lambda->GetFunctionArguments()[0];
  auto storeData = lambda->GetFunctionArguments()[1];
  auto memoryStateArgument = lambda->GetFunctionArguments()[2];
  auto storeOutput =
      StoreNonVolatileNode::Create(storeAddress, storeData, { memoryStateArgument }, 32);

  auto lambdaOutput = lambda->finalize({ storeOutput[0] });
  jlm::llvm::GraphExport::Create(*lambdaOutput, "f");

  // Act
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);
  MemoryConverter(*rvsdgModule);
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);

  // Memory Converter replaces the lambda so we start from the root of the graph
  auto region = &rvsdgModule->Rvsdg().GetRootRegion();
  assert(region->nnodes() == 1);
  lambda = jlm::util::AssertedCast<jlm::rvsdg::LambdaNode>(region->Nodes().begin().ptr());

  // Assert
  auto lambdaRegion = lambda->subregion();
  assert(lambdaRegion->nnodes() == 2);
  assert(lambdaRegion->narguments() == 4);
  assert(lambdaRegion->nresults() == 2);

  // Memory state
  assert(is<MemoryStateType>(lambdaRegion->result(0)->origin()->type()));

  // Store
  auto storeNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(lambdaRegion->result(0)->origin())->node();
  assert(is<store_op>(storeNode));

  // Request Node
  auto requestNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(lambdaRegion->result(1)->origin())->node();
  assert(is<mem_req_op>(requestNode));

  // Request source
  auto requestSource = requestNode->input(0)->origin();
  storeNode = jlm::util::AssertedCast<jlm::rvsdg::node_output>(requestSource)->node();
  assert(is<store_op>(storeNode));

  return 0;
}
JLM_UNIT_TEST_REGISTER("jlm/hls/backend/rvsdg2rhls/MemoryConverterTests-Store", TestStore)

static int
TestLoadStore()
{
  using namespace jlm::llvm;
  using namespace jlm::hls;

  auto rvsdgModule = RvsdgModule::Create(jlm::util::filepath(""), "", "");

  // Setup the function
  std::cout << "Function Setup" << std::endl;
  auto functionType = jlm::rvsdg::FunctionType::Create(
      { jlm::llvm::PointerType::Create(),
        jlm::rvsdg::bittype::Create(32),
        MemoryStateType::Create() },
      { MemoryStateType::Create() });

  auto lambda = jlm::rvsdg::LambdaNode::Create(
      rvsdgModule->Rvsdg().GetRootRegion(),
      LlvmLambdaOperation::Create(functionType, "test", linkage::external_linkage));

  // Load followed by store
  auto loadAddress = lambda->GetFunctionArguments()[0];
  auto storeData = lambda->GetFunctionArguments()[1];
  auto memoryStateArgument = lambda->GetFunctionArguments()[2];
  auto loadOutput = LoadNonVolatileOperation::Create(
      loadAddress,
      { memoryStateArgument },
      jlm::llvm::PointerType::Create(),
      32);
  auto storeOutput = StoreNonVolatileNode::Create(loadOutput[0], storeData, { loadOutput[1] }, 32);

  auto lambdaOutput = lambda->finalize({ storeOutput[0] });
  jlm::llvm::GraphExport::Create(*lambdaOutput, "f");

  // Act
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);
  MemoryConverter(*rvsdgModule);
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);

  // Memory Converter replaces the lambda so we start from the root of the graph
  auto region = &rvsdgModule->Rvsdg().GetRootRegion();
  assert(region->nnodes() == 1);
  lambda = jlm::util::AssertedCast<jlm::rvsdg::LambdaNode>(region->Nodes().begin().ptr());

  // Assert
  auto lambdaRegion = lambda->subregion();
  assert(lambdaRegion->nnodes() == 5);
  assert(lambdaRegion->narguments() == 5);
  assert(lambdaRegion->nresults() == 3);

  // Memory state
  std::cout << lambdaRegion->result(0)->origin()->type().debug_string() << std::endl;
  assert(is<MemoryStateType>(lambdaRegion->result(0)->origin()->type()));

  // Store Node
  auto storeNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(lambdaRegion->result(0)->origin())->node();
  assert(is<store_op>(storeNode));

  // Request Node
  auto firstRequestNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(lambdaRegion->result(1)->origin())->node();
  assert(is<mem_req_op>(firstRequestNode));

  // Request Node
  auto secondRequestNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(lambdaRegion->result(2)->origin())->node();
  assert(is<mem_req_op>(secondRequestNode));

  // Load node
  auto loadNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(storeNode->input(0)->origin())->node();
  assert(is<load_op>(loadNode));

  // Response Node
  auto responseNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(loadNode->input(2)->origin())->node();
  assert(is<mem_resp_op>(responseNode));

  return 0;
}
JLM_UNIT_TEST_REGISTER("jlm/hls/backend/rvsdg2rhls/MemoryConverterTests-LoadStore", TestLoadStore)

static int
TestThetaLoad()
{
  using namespace jlm::llvm;
  using namespace jlm::hls;

  auto rvsdgModule = RvsdgModule::Create(jlm::util::filepath(""), "", "");

  // Setup the function
  std::cout << "Function Setup" << std::endl;
  auto functionType = jlm::rvsdg::FunctionType::Create(
      { jlm::rvsdg::bittype::Create(32),
        jlm::rvsdg::bittype::Create(32),
        jlm::rvsdg::bittype::Create(32),
        jlm::llvm::PointerType::Create(),
        MemoryStateType::Create() },
      { jlm::llvm::PointerType::Create(), MemoryStateType::Create() });

  auto lambda = jlm::rvsdg::LambdaNode::Create(
      rvsdgModule->Rvsdg().GetRootRegion(),
      LlvmLambdaOperation::Create(functionType, "test", linkage::external_linkage));

  // Theta
  auto theta = jlm::rvsdg::ThetaNode::create(lambda->subregion());
  // Predicate
  auto idv = theta->AddLoopVar(lambda->GetFunctionArguments()[0]);
  auto lvs = theta->AddLoopVar(lambda->GetFunctionArguments()[1]);
  auto lve = theta->AddLoopVar(lambda->GetFunctionArguments()[2]);
  auto arm = jlm::rvsdg::CreateOpNode<jlm::rvsdg::bitadd_op>({ idv.pre, lvs.pre }, 32).output(0);
  auto cmp = jlm::rvsdg::CreateOpNode<jlm::rvsdg::bitult_op>({ arm, lve.pre }, 32).output(0);
  auto match = jlm::rvsdg::match(1, { { 1, 1 } }, 0, 2, cmp);
  idv.post->divert_to(arm);
  theta->set_predicate(match);

  // Load node
  auto loadAddress = theta->AddLoopVar(lambda->GetFunctionArguments()[3]);
  auto memoryStateArgument = theta->AddLoopVar(lambda->GetFunctionArguments()[4]);
  auto loadOutput = LoadNonVolatileOperation::Create(
      loadAddress.pre,
      { memoryStateArgument.pre },
      PointerType::Create(),
      32);
  loadAddress.post->divert_to(loadOutput[0]);
  memoryStateArgument.post->divert_to(loadOutput[1]);

  auto lambdaOutput = lambda->finalize({ theta->output(3), theta->output(4) });
  GraphExport::Create(*lambdaOutput, "f");

  auto lambdaRegion = lambda->subregion();
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);

  // Act
  mem_sep_argument(*rvsdgModule);

  // Assert
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);
  auto * const entryMemoryStateSplitInput = *lambdaRegion->argument(4)->begin();
  auto * entryMemoryStateSplitNode = jlm::rvsdg::input::GetNode(*entryMemoryStateSplitInput);
  assert(is<LambdaEntryMemoryStateSplitOperation>(entryMemoryStateSplitNode));
  auto exitMemoryStateMergeNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(lambdaRegion->result(1)->origin())->node();
  assert(is<LambdaExitMemoryStateMergeOperation>(exitMemoryStateMergeNode));

  // Act
  ConvertThetaNodes(*rvsdgModule);
  // Simple assert as ConvertThetaNodes() is tested in separate unit tests
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);
  assert(jlm::rvsdg::Region::Contains<loop_op>(*lambdaRegion, true));

  // Act
  mem_queue(*rvsdgModule);
  // Simple assert as mem_queue() is tested in separate unit tests
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);
  assert(jlm::rvsdg::Region::Contains<state_gate_op>(*lambdaRegion, true));
  assert(jlm::rvsdg::Region::Contains<MemoryStateSplitOperation>(*lambdaRegion, true));
  assert(jlm::rvsdg::Region::Contains<MemoryStateMergeOperation>(*lambdaRegion, true));

  // Act
  MemoryConverter(*rvsdgModule);

  // Assert
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);

  // Memory Converter replaces the lambda so we start from the root of the graph
  auto region = &rvsdgModule->Rvsdg().GetRootRegion();
  assert(region->nnodes() == 1);
  lambda = jlm::util::AssertedCast<jlm::rvsdg::LambdaNode>(region->Nodes().begin().ptr());
  lambdaRegion = lambda->subregion();

  assert(jlm::rvsdg::Region::Contains<mem_resp_op>(*lambdaRegion, true));
  assert(jlm::rvsdg::Region::Contains<mem_req_op>(*lambdaRegion, true));

  // Request Node
  auto requestNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(lambdaRegion->result(2)->origin())->node();
  assert(is<mem_req_op>(requestNode));

  // HLS_LOOP Node
  auto loopOutput =
      jlm::util::AssertedCast<const jlm::rvsdg::StructuralOutput>(requestNode->input(0)->origin());
  auto loopNode = jlm::util::AssertedCast<const jlm::rvsdg::StructuralNode>(loopOutput->node());
  assert(is<loop_op>(loopNode));
  // Loop Result
  auto & thetaResult = loopOutput->results;
  assert(thetaResult.size() == 1);
  // Load Node
  auto loadNode =
      jlm::util::AssertedCast<const jlm::rvsdg::node_output>(thetaResult.first()->origin())->node();
  assert(is<decoupled_load_op>(loadNode));
  // Loop Argument
  auto thetaArgument =
      jlm::util::AssertedCast<const jlm::rvsdg::RegionArgument>(loadNode->input(1)->origin());
  auto thetaInput = thetaArgument->input();

  // Response Node
  auto responseNode =
      jlm::util::AssertedCast<const jlm::rvsdg::node_output>(thetaInput->origin())->node();
  assert(is<mem_resp_op>(responseNode));

  // Lambda argument
  assert(is<jlm::rvsdg::RegionArgument>(responseNode->input(0)->origin()));

  return 0;
}
JLM_UNIT_TEST_REGISTER("jlm/hls/backend/rvsdg2rhls/MemoryConverterTests-ThetaLoad", TestThetaLoad)

static int
TestThetaStore()
{
  using namespace jlm::llvm;
  using namespace jlm::hls;

  auto rvsdgModule = RvsdgModule::Create(jlm::util::filepath(""), "", "");

  // Setup the function
  std::cout << "Function Setup" << std::endl;
  auto functionType = jlm::rvsdg::FunctionType::Create(
      { jlm::rvsdg::bittype::Create(32),
        jlm::rvsdg::bittype::Create(32),
        jlm::rvsdg::bittype::Create(32),
        jlm::llvm::PointerType::Create(),
        jlm::rvsdg::bittype::Create(32),
        MemoryStateType::Create() },
      { MemoryStateType::Create() });

  auto lambda = jlm::rvsdg::LambdaNode::Create(
      rvsdgModule->Rvsdg().GetRootRegion(),
      LlvmLambdaOperation::Create(functionType, "test", linkage::external_linkage));

  // Theta
  auto theta = jlm::rvsdg::ThetaNode::create(lambda->subregion());
  // Predicate
  auto idv = theta->AddLoopVar(lambda->GetFunctionArguments()[0]);
  auto lvs = theta->AddLoopVar(lambda->GetFunctionArguments()[1]);
  auto lve = theta->AddLoopVar(lambda->GetFunctionArguments()[2]);
  auto arm = jlm::rvsdg::CreateOpNode<jlm::rvsdg::bitadd_op>({ idv.pre, lvs.pre }, 32).output(0);
  auto cmp = jlm::rvsdg::CreateOpNode<jlm::rvsdg::bitult_op>({ arm, lve.pre }, 32).output(0);
  auto match = jlm::rvsdg::match(1, { { 1, 1 } }, 0, 2, cmp);
  idv.post->divert_to(arm);
  theta->set_predicate(match);

  // Store node
  auto storeAddress = theta->AddLoopVar(lambda->GetFunctionArguments()[3]);
  auto storeData = theta->AddLoopVar(lambda->GetFunctionArguments()[4]);
  auto memoryStateArgument = theta->AddLoopVar(lambda->GetFunctionArguments()[5]);
  auto storeOutput = StoreNonVolatileNode::Create(
      storeAddress.pre,
      storeData.pre,
      { memoryStateArgument.pre },
      32);
  memoryStateArgument.post->divert_to(storeOutput[0]);

  auto lambdaOutput = lambda->finalize({ theta->output(5) });
  GraphExport::Create(*lambdaOutput, "f");

  auto lambdaRegion = lambda->subregion();
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);

  // Act
  mem_sep_argument(*rvsdgModule);

  // Assert
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);
  auto * const entryMemoryStateSplitInput = *lambdaRegion->argument(5)->begin();
  auto * entryMemoryStateSplitNode = jlm::rvsdg::input::GetNode(*entryMemoryStateSplitInput);
  assert(is<LambdaEntryMemoryStateSplitOperation>(entryMemoryStateSplitNode));
  auto exitMemoryStateMergeNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(lambdaRegion->result(0)->origin())->node();
  assert(is<LambdaExitMemoryStateMergeOperation>(exitMemoryStateMergeNode));

  // Act
  ConvertThetaNodes(*rvsdgModule);
  // Simple assert as ConvertThetaNodes() is tested in separate unit tests
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);
  assert(jlm::rvsdg::Region::Contains<loop_op>(*lambdaRegion, true));

  // Act
  mem_queue(*rvsdgModule);
  // Simple assert as mem_queue() is tested in separate unit tests
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);
  assert(jlm::rvsdg::Region::Contains<MemoryStateSplitOperation>(*lambdaRegion, true));
  assert(jlm::rvsdg::Region::Contains<MemoryStateMergeOperation>(*lambdaRegion, true));

  // Act
  MemoryConverter(*rvsdgModule);

  // Assert
  jlm::rvsdg::view(rvsdgModule->Rvsdg(), stdout);

  // Memory Converter replaces the lambda so we start from the root of the graph
  auto region = &rvsdgModule->Rvsdg().GetRootRegion();
  assert(region->nnodes() == 1);
  lambda = jlm::util::AssertedCast<jlm::rvsdg::LambdaNode>(region->Nodes().begin().ptr());
  lambdaRegion = lambda->subregion();

  assert(jlm::rvsdg::Region::Contains<mem_req_op>(*lambdaRegion, true));

  // Request Node
  auto requestNode =
      jlm::util::AssertedCast<jlm::rvsdg::node_output>(lambdaRegion->result(1)->origin())->node();
  assert(is<mem_req_op>(requestNode));

  // HLS_LOOP Node
  auto loopOutput =
      jlm::util::AssertedCast<const jlm::rvsdg::StructuralOutput>(requestNode->input(0)->origin());
  auto loopNode = jlm::util::AssertedCast<const jlm::rvsdg::StructuralNode>(loopOutput->node());
  assert(is<loop_op>(loopNode));
  // Loop Result
  auto & thetaResult = loopOutput->results;
  assert(thetaResult.size() == 1);
  // Load Node
  auto storeNode =
      jlm::util::AssertedCast<const jlm::rvsdg::node_output>(thetaResult.first()->origin())->node();
  assert(is<store_op>(storeNode));
  // NDMux Node
  auto ndMuxNode =
      jlm::util::AssertedCast<const jlm::rvsdg::node_output>(storeNode->input(2)->origin())->node();
  assert(is<mux_op>(ndMuxNode));
  // Loop Argument
  assert(is<jlm::rvsdg::RegionArgument>(ndMuxNode->input(2)->origin()));

  return 0;
}
JLM_UNIT_TEST_REGISTER("jlm/hls/backend/rvsdg2rhls/MemoryConverterTests-ThetaStore", TestThetaStore)
