/*
 * Copyright 2021 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <test-operation.hpp>
#include <test-registry.hpp>
#include <test-types.hpp>

#include <jlm/llvm/ir/operators.hpp>
#include <jlm/llvm/ir/RvsdgModule.hpp>

#include <jlm/rvsdg/view.hpp>

static void
TestCopy()
{
  using namespace jlm::llvm;

  // Arrange
  jlm::tests::valuetype valueType;
  iostatetype iOStateType;
  MemoryStateType memoryStateType;
  FunctionType functionType(
      { &valueType, &iOStateType, &memoryStateType },
      { &valueType, &iOStateType, &memoryStateType });

  jlm::rvsdg::graph rvsdg;
  auto function1 = rvsdg.add_import({ PointerType(), "function1" });
  auto value1 = rvsdg.add_import({ valueType, "value1" });
  auto iOState1 = rvsdg.add_import({ iOStateType, "iOState1" });
  auto memoryState1 = rvsdg.add_import({ memoryStateType, "memoryState1" });

  auto function2 = rvsdg.add_import({ PointerType(), "function2" });
  auto value2 = rvsdg.add_import({ valueType, "value2" });
  auto iOState2 = rvsdg.add_import({ iOStateType, "iOState2" });
  auto memoryState2 = rvsdg.add_import({ memoryStateType, "memoryState2" });

  auto callResults = CallNode::Create(function1, functionType, { value1, iOState1, memoryState1 });

  // Act
  auto node = jlm::rvsdg::node_output::node(callResults[0]);
  auto callNode = jlm::util::AssertedCast<const CallNode>(node);
  auto copiedNode = callNode->copy(rvsdg.root(), { function2, value2, iOState2, memoryState2 });

  // Assert
  auto copiedCallNode = dynamic_cast<const CallNode *>(copiedNode);
  assert(copiedNode != nullptr);
  assert(callNode->GetOperation() == copiedCallNode->GetOperation());
}

static void
TestCallNodeAccessors()
{
  using namespace jlm::llvm;

  // Arrange
  jlm::tests::valuetype valueType;
  iostatetype iOStateType;
  MemoryStateType memoryStateType;
  FunctionType functionType(
      { &valueType, &iOStateType, &memoryStateType },
      { &valueType, &iOStateType, &memoryStateType });

  jlm::rvsdg::graph rvsdg;
  auto f = rvsdg.add_import({ PointerType(), "function" });
  auto v = rvsdg.add_import({ valueType, "value" });
  auto i = rvsdg.add_import({ iOStateType, "IOState" });
  auto m = rvsdg.add_import({ memoryStateType, "memoryState" });

  // Act
  auto results = CallNode::Create(f, functionType, { v, i, m });
  auto & callNode = *jlm::util::AssertedCast<CallNode>(jlm::rvsdg::node_output::node(results[0]));

  // Assert
  assert(callNode.NumArguments() == 3);
  assert(callNode.NumArguments() == callNode.ninputs() - 1);
  assert(callNode.Argument(0)->origin() == v);
  assert(callNode.Argument(1)->origin() == i);
  assert(callNode.Argument(2)->origin() == m);

  assert(callNode.NumResults() == 3);
  assert(callNode.Result(0)->type() == valueType);
  assert(callNode.Result(1)->type() == iOStateType);
  assert(callNode.Result(2)->type() == memoryStateType);

  assert(callNode.GetFunctionInput()->origin() == f);
  assert(callNode.GetIoStateInput()->origin() == i);
  assert(callNode.GetMemoryStateInput()->origin() == m);

  assert(callNode.GetIoStateOutput()->type() == iOStateType);
  assert(callNode.GetMemoryStateOutput()->type() == memoryStateType);
}

static void
TestCallTypeClassifierIndirectCall()
{
  using namespace jlm::llvm;

  // Arrange
  jlm::tests::valuetype vt;
  iostatetype iOStateType;
  MemoryStateType memoryStateType;
  FunctionType fcttype1(
      { &iOStateType, &memoryStateType },
      { &vt, &iOStateType, &memoryStateType });
  PointerType pt;
  FunctionType fcttype2(
      { &pt, &iOStateType, &memoryStateType },
      { &vt, &iOStateType, &memoryStateType });

  auto module = RvsdgModule::Create(jlm::util::filepath(""), "", "");
  auto graph = &module->Rvsdg();

  auto nf = graph->node_normal_form(typeid(jlm::rvsdg::operation));
  nf->set_mutable(false);

  auto SetupFunction = [&]()
  {
    auto lambda = lambda::node::create(graph->root(), fcttype2, "fct", linkage::external_linkage);
    auto iOStateArgument = lambda->fctargument(1);
    auto memoryStateArgument = lambda->fctargument(2);

    auto one = jlm::rvsdg::create_bitconstant(lambda->subregion(), 32, 1);

    auto alloca = alloca_op::create(pt, one, 8);

    auto store = StoreNode::Create(alloca[0], lambda->fctargument(0), { alloca[1] }, 8);

    auto load = LoadNode::Create(alloca[0], store, pt, 8);

    auto callResults =
        CallNode::Create(load[0], fcttype1, { iOStateArgument, memoryStateArgument });

    lambda->finalize(callResults);

    graph->add_export(lambda->output(), { pt, "f" });

    return std::make_tuple(
        jlm::util::AssertedCast<CallNode>(jlm::rvsdg::node_output::node(callResults[0])),
        load[0]);
  };

  auto [callNode, loadOutput] = SetupFunction();

  // Act
  auto callTypeClassifier = CallNode::ClassifyCall(*callNode);

  // Assert
  assert(callTypeClassifier->IsIndirectCall());
  assert(loadOutput == &callTypeClassifier->GetFunctionOrigin());
}

static void
TestCallTypeClassifierNonRecursiveDirectCall()
{
  // Arrange
  using namespace jlm::llvm;

  auto module = RvsdgModule::Create(jlm::util::filepath(""), "", "");
  auto graph = &module->Rvsdg();

  auto nf = graph->node_normal_form(typeid(jlm::rvsdg::operation));
  nf->set_mutable(false);

  jlm::tests::valuetype vt;
  iostatetype iOStateType;
  MemoryStateType memoryStateType;

  FunctionType functionTypeG(
      { &iOStateType, &memoryStateType },
      { &vt, &iOStateType, &memoryStateType });

  auto SetupFunctionG = [&]()
  {
    auto lambda =
        lambda::node::create(graph->root(), functionTypeG, "g", linkage::external_linkage);
    auto iOStateArgument = lambda->fctargument(0);
    auto memoryStateArgument = lambda->fctargument(1);

    auto constant = jlm::tests::test_op::create(lambda->subregion(), {}, { &vt });

    auto lambdaOutput =
        lambda->finalize({ constant->output(0), iOStateArgument, memoryStateArgument });

    return lambdaOutput;
  };

  auto SetupFunctionF = [&](lambda::output * g)
  {
    auto SetupOuterTheta = [](jlm::rvsdg::region * region, jlm::rvsdg::argument * functionG)
    {
      auto outerTheta = jlm::rvsdg::theta_node::create(region);
      auto otf = outerTheta->add_loopvar(functionG);

      auto innerTheta = jlm::rvsdg::theta_node::create(outerTheta->subregion());
      auto itf = innerTheta->add_loopvar(otf->argument());

      auto predicate = jlm::rvsdg::control_false(innerTheta->subregion());
      auto gamma = jlm::rvsdg::gamma_node::create(predicate, 2);
      auto ev = gamma->add_entryvar(itf->argument());
      auto xv = gamma->add_exitvar({ ev->argument(0), ev->argument(1) });

      itf->result()->divert_to(xv);
      otf->result()->divert_to(itf);

      return otf;
    };

    jlm::tests::valuetype vt;
    iostatetype iOStateType;
    MemoryStateType memoryStateType;

    FunctionType functionType(
        { &iOStateType, &memoryStateType },
        { &vt, &iOStateType, &memoryStateType });

    auto lambda = lambda::node::create(graph->root(), functionType, "f", linkage::external_linkage);
    auto functionGArgument = lambda->add_ctxvar(g);
    auto iOStateArgument = lambda->fctargument(0);
    auto memoryStateArgument = lambda->fctargument(1);

    auto functionG = SetupOuterTheta(lambda->subregion(), functionGArgument);

    auto callResults =
        CallNode::Create(functionG, functionTypeG, { iOStateArgument, memoryStateArgument });

    lambda->finalize(callResults);

    return std::make_tuple(
        lambda,
        jlm::util::AssertedCast<CallNode>(jlm::rvsdg::node_output::node(callResults[0])));
  };

  auto g = SetupFunctionG();
  auto [f, callNode] = SetupFunctionF(g);

  graph->add_export(f->output(), { PointerType(), "f" });

  //	jlm::rvsdg::view(graph->root(), stdout);

  // Act
  auto callTypeClassifier = CallNode::ClassifyCall(*callNode);

  // Assert
  assert(callTypeClassifier->IsNonRecursiveDirectCall());
  assert(&callTypeClassifier->GetLambdaOutput() == g);
}

static void
TestCallTypeClassifierNonRecursiveDirectCallTheta()
{
  using namespace jlm::llvm;

  // Arrange
  auto module = RvsdgModule::Create(jlm::util::filepath(""), "", "");
  auto graph = &module->Rvsdg();

  auto nf = graph->node_normal_form(typeid(jlm::rvsdg::operation));
  nf->set_mutable(false);

  jlm::tests::valuetype vt;
  iostatetype iOStateType;
  MemoryStateType memoryStateType;

  FunctionType functionTypeG(
      { &iOStateType, &memoryStateType },
      { &vt, &iOStateType, &memoryStateType });

  auto SetupFunctionG = [&]()
  {
    auto lambda =
        lambda::node::create(graph->root(), functionTypeG, "g", linkage::external_linkage);
    auto iOStateArgument = lambda->fctargument(0);
    auto memoryStateArgument = lambda->fctargument(1);

    auto c1 = jlm::tests::test_op::create(lambda->subregion(), {}, { &vt });

    return lambda->finalize({ c1->output(0), iOStateArgument, memoryStateArgument });
  };

  auto SetupFunctionF = [&](lambda::output * g)
  {
    auto SetupOuterTheta = [&](jlm::rvsdg::region * region,
                               jlm::rvsdg::argument * g,
                               jlm::rvsdg::output * value,
                               jlm::rvsdg::output * iOState,
                               jlm::rvsdg::output * memoryState)
    {
      auto SetupInnerTheta = [&](jlm::rvsdg::region * region, jlm::rvsdg::argument * g)
      {
        auto innerTheta = jlm::rvsdg::theta_node::create(region);
        auto thetaOutputG = innerTheta->add_loopvar(g);

        return thetaOutputG;
      };

      auto outerTheta = jlm::rvsdg::theta_node::create(region);
      auto thetaOutputG = outerTheta->add_loopvar(g);
      auto thetaOutputValue = outerTheta->add_loopvar(value);
      auto thetaOutputIoState = outerTheta->add_loopvar(iOState);
      auto thetaOutputMemoryState = outerTheta->add_loopvar(memoryState);

      auto functionG = SetupInnerTheta(outerTheta->subregion(), thetaOutputG->argument());

      auto callResults = CallNode::Create(
          functionG,
          functionTypeG,
          { thetaOutputIoState->argument(), thetaOutputMemoryState->argument() });

      thetaOutputG->result()->divert_to(functionG);
      thetaOutputValue->result()->divert_to(callResults[0]);
      thetaOutputIoState->result()->divert_to(callResults[1]);
      thetaOutputMemoryState->result()->divert_to(callResults[2]);

      return std::make_tuple(
          thetaOutputValue,
          thetaOutputIoState,
          thetaOutputMemoryState,
          jlm::util::AssertedCast<CallNode>(jlm::rvsdg::node_output::node(callResults[0])));
    };

    jlm::tests::valuetype vt;
    iostatetype iOStateType;
    MemoryStateType memoryStateType;

    FunctionType functionType(
        { &iOStateType, &memoryStateType },
        { &vt, &iOStateType, &memoryStateType });

    auto lambda = lambda::node::create(graph->root(), functionType, "f", linkage::external_linkage);
    auto functionG = lambda->add_ctxvar(g);
    auto iOStateArgument = lambda->fctargument(0);
    auto memoryStateArgument = lambda->fctargument(1);

    auto value = jlm::tests::test_op::create(lambda->subregion(), {}, { &vt })->output(0);

    auto [loopValue, iOState, memoryState, callNode] = SetupOuterTheta(
        lambda->subregion(),
        functionG,
        value,
        iOStateArgument,
        memoryStateArgument);

    auto lambdaOutput = lambda->finalize({ loopValue, iOState, memoryState });

    return std::make_tuple(lambdaOutput, callNode);
  };

  auto g = SetupFunctionG();
  auto [f, callNode] = SetupFunctionF(g);
  graph->add_export(f, { PointerType(), "f" });

  jlm::rvsdg::view(graph->root(), stdout);

  // Act
  auto callTypeClassifier = CallNode::ClassifyCall(*callNode);

  // Assert
  assert(callTypeClassifier->IsNonRecursiveDirectCall());
  assert(&callTypeClassifier->GetLambdaOutput() == g);
}

static void
TestCallTypeClassifierRecursiveDirectCall()
{
  // Arrange
  using namespace jlm::llvm;

  auto module = RvsdgModule::Create(jlm::util::filepath(""), "", "");
  auto graph = &module->Rvsdg();

  auto nf = graph->node_normal_form(typeid(jlm::rvsdg::operation));
  nf->set_mutable(false);

  auto SetupFib = [&]()
  {
    PointerType pbit64;
    iostatetype iOStateType;
    MemoryStateType memoryStateType;
    FunctionType functionType(
        { &jlm::rvsdg::bit64, &pbit64, &iOStateType, &memoryStateType },
        { &iOStateType, &memoryStateType });
    PointerType pt;

    jlm::llvm::phi::builder pb;
    pb.begin(graph->root());
    auto fibrv = pb.add_recvar(pt);

    auto lambda =
        lambda::node::create(pb.subregion(), functionType, "fib", linkage::external_linkage);
    auto valueArgument = lambda->fctargument(0);
    auto pointerArgument = lambda->fctargument(1);
    auto iOStateArgument = lambda->fctargument(2);
    auto memoryStateArgument = lambda->fctargument(3);
    auto ctxVarFib = lambda->add_ctxvar(fibrv->argument());

    auto two = jlm::rvsdg::create_bitconstant(lambda->subregion(), 64, 2);
    auto bitult = jlm::rvsdg::bitult_op::create(64, valueArgument, two);
    auto predicate = jlm::rvsdg::match(1, { { 0, 1 } }, 0, 2, bitult);

    auto gammaNode = jlm::rvsdg::gamma_node::create(predicate, 2);
    auto nev = gammaNode->add_entryvar(valueArgument);
    auto resultev = gammaNode->add_entryvar(pointerArgument);
    auto fibev = gammaNode->add_entryvar(ctxVarFib);
    auto gIIoState = gammaNode->add_entryvar(iOStateArgument);
    auto gIMemoryState = gammaNode->add_entryvar(memoryStateArgument);

    /* gamma subregion 0 */
    auto one = jlm::rvsdg::create_bitconstant(gammaNode->subregion(0), 64, 1);
    auto nm1 = jlm::rvsdg::bitsub_op::create(64, nev->argument(0), one);
    auto callfibm1Results = CallNode::Create(
        fibev->argument(0),
        functionType,
        { nm1, resultev->argument(0), gIIoState->argument(0), gIMemoryState->argument(0) });

    two = jlm::rvsdg::create_bitconstant(gammaNode->subregion(0), 64, 2);
    auto nm2 = jlm::rvsdg::bitsub_op::create(64, nev->argument(0), two);
    auto callfibm2Results = CallNode::Create(
        fibev->argument(0),
        functionType,
        { nm2, resultev->argument(0), callfibm1Results[0], callfibm1Results[1] });

    auto gepnm1 =
        GetElementPtrOperation::Create(resultev->argument(0), { nm1 }, jlm::rvsdg::bit64, pbit64);
    auto ldnm1 = LoadNode::Create(gepnm1, { callfibm2Results[1] }, jlm::rvsdg::bit64, 8);

    auto gepnm2 =
        GetElementPtrOperation::Create(resultev->argument(0), { nm2 }, jlm::rvsdg::bit64, pbit64);
    auto ldnm2 = LoadNode::Create(gepnm2, { ldnm1[1] }, jlm::rvsdg::bit64, 8);

    auto sum = jlm::rvsdg::bitadd_op::create(64, ldnm1[0], ldnm2[0]);

    /* gamma subregion 1 */
    /* Nothing needs to be done */

    auto sumex = gammaNode->add_exitvar({ sum, nev->argument(1) });
    auto gOIoState = gammaNode->add_exitvar({ callfibm2Results[0], gIIoState->argument(1) });
    auto gOMemoryState = gammaNode->add_exitvar({ ldnm2[1], gIMemoryState->argument(1) });

    auto gepn = GetElementPtrOperation::Create(
        pointerArgument,
        { valueArgument },
        jlm::rvsdg::bit64,
        pbit64);
    auto store = StoreNode::Create(gepn, sumex, { gOMemoryState }, 8);

    auto lambdaOutput = lambda->finalize({ gOIoState, store[0] });

    fibrv->result()->divert_to(lambdaOutput);
    pb.end();

    graph->add_export(fibrv, { pt, "fib" });

    return std::make_tuple(
        lambdaOutput,
        jlm::util::AssertedCast<CallNode>(jlm::rvsdg::node_output::node(callfibm1Results[0])),
        jlm::util::AssertedCast<CallNode>(jlm::rvsdg::node_output::node(callfibm2Results[0])));
  };

  auto [fibfct, callFib1, callFib2] = SetupFib();

  // Act
  auto callTypeClassifier1 = CallNode::ClassifyCall(*callFib1);
  auto callTypeClassifier2 = CallNode::ClassifyCall(*callFib2);

  // Assert
  assert(callTypeClassifier1->IsRecursiveDirectCall());
  assert(&callTypeClassifier1->GetLambdaOutput() == fibfct);

  assert(callTypeClassifier2->IsRecursiveDirectCall());
  assert(&callTypeClassifier2->GetLambdaOutput() == fibfct);
}

static int
Test()
{
  TestCopy();

  TestCallNodeAccessors();
  TestCallTypeClassifierIndirectCall();
  TestCallTypeClassifierNonRecursiveDirectCall();
  TestCallTypeClassifierNonRecursiveDirectCallTheta();
  TestCallTypeClassifierRecursiveDirectCall();

  return 0;
}

JLM_UNIT_TEST_REGISTER("jlm/llvm/ir/operators/TestCall", Test)
