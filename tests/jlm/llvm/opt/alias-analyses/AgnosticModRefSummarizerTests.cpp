/*
 * Copyright 2022 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "TestRvsdgs.hpp"

#include <test-registry.hpp>

#include <jlm/rvsdg/view.hpp>

#include <jlm/llvm/opt/alias-analyses/AgnosticModRefSummarizer.hpp>
#include <jlm/llvm/opt/alias-analyses/Steensgaard.hpp>
#include <jlm/util/Statistics.hpp>

#include <iostream>

static std::unique_ptr<jlm::llvm::aa::PointsToGraph>
RunSteensgaard(const jlm::llvm::RvsdgModule & module)
{
  using namespace jlm::llvm;

  aa::Steensgaard stgd;
  jlm::util::StatisticsCollector statisticsCollector;
  return stgd.Analyze(module, statisticsCollector);
}

static void
TestStore1()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::StoreTest1 & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda).Size();
    auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda).Size();

    assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
    assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
  };

  jlm::tests::StoreTest1 test;
  // jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*PointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestStore2()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::StoreTest2 & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda).Size();
    auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda).Size();

    assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
    assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
  };

  jlm::tests::StoreTest2 test;
  // jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*PointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestLoad1()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::LoadTest1 & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda).Size();
    auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda).Size();

    assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
    assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
  };

  jlm::tests::LoadTest1 test;
  // jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*PointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestLoad2()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::LoadTest2 & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda).Size();
    auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda).Size();

    assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
    assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
  };

  jlm::tests::LoadTest2 test;
  // jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestLoadFromUndef()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::LoadFromUndefTest & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(test.Lambda()).Size();
    auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(test.Lambda()).Size();

    assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
    assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
  };

  jlm::tests::LoadFromUndefTest test;
  // jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*pointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestCall1()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::CallTest1 & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    /*
     * Validate function f
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_f).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_f).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
    }

    /*
     * Validate function g
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_g).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_g).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
    }

    /*
     * Validate function h
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_h).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_h).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());

      auto numCallFEntryNodes = modRefSummary.GetCallEntryNodes(test.CallF()).Size();
      auto numCallFExitNodes = modRefSummary.GetCallExitNodes(test.CallF()).Size();

      assert(numCallFEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallFExitNodes == pointsToGraph.NumMemoryNodes());

      auto numCallGEntryNodes = modRefSummary.GetCallEntryNodes(test.CallG()).Size();
      auto numCallGExitNodes = modRefSummary.GetCallExitNodes(test.CallG()).Size();

      assert(numCallGEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallGExitNodes == pointsToGraph.NumMemoryNodes());
    }
  };

  jlm::tests::CallTest1 test;
  //	jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*PointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestCall2()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::CallTest2 & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    /*
     * Validate function create
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_create).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_create).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
    }

    /*
     * Validate function destroy
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_destroy).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_destroy).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
    }

    /*
     * Validate function test
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_test).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_test).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());

      auto numCallCreate1EntryNodes = modRefSummary.GetCallEntryNodes(test.CallCreate1()).Size();
      auto numCallCreate1ExitNodes = modRefSummary.GetCallExitNodes(test.CallCreate1()).Size();

      assert(numCallCreate1EntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallCreate1ExitNodes == pointsToGraph.NumMemoryNodes());

      auto numCallCreate2EntryNodes = modRefSummary.GetCallEntryNodes(test.CallCreate2()).Size();
      auto numCallCreate2ExitNodes = modRefSummary.GetCallExitNodes(test.CallCreate2()).Size();

      assert(numCallCreate2EntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallCreate2ExitNodes == pointsToGraph.NumMemoryNodes());

      auto numCallDestroy1EntryNodes = modRefSummary.GetCallEntryNodes(test.CallDestroy1()).Size();
      auto numCallDestroy1ExitNodes = modRefSummary.GetCallExitNodes(test.CallDestroy1()).Size();

      assert(numCallDestroy1EntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallDestroy1ExitNodes == pointsToGraph.NumMemoryNodes());

      auto numCallDestroy2EntryNodes = modRefSummary.GetCallEntryNodes(test.CallDestroy2()).Size();
      auto numCallDestroy2ExitNodes = modRefSummary.GetCallExitNodes(test.CallDestroy2()).Size();

      assert(numCallDestroy2EntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallDestroy2ExitNodes == pointsToGraph.NumMemoryNodes());
    }
  };

  jlm::tests::CallTest2 test;
  //	jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*PointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestIndirectCall()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::IndirectCallTest1 & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    /*
     * Validate function four
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(test.GetLambdaFour()).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(test.GetLambdaFour()).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
    }

    /*
     * Validate function three
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(test.GetLambdaThree()).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(test.GetLambdaThree()).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
    }

    /*
     * Validate function indcall
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(test.GetLambdaIndcall()).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(test.GetLambdaIndcall()).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());

      auto numCallIndcallEntryNodes = modRefSummary.GetCallEntryNodes(test.CallIndcall()).Size();
      auto numCallIndcallExitNodes = modRefSummary.GetCallExitNodes(test.CallIndcall()).Size();

      assert(numCallIndcallEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallIndcallExitNodes == pointsToGraph.NumMemoryNodes());
    }

    /*
     * Validate function test
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(test.GetLambdaTest()).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(test.GetLambdaTest()).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());

      auto numCallThreeEntryNodes = modRefSummary.GetCallEntryNodes(test.CallThree()).Size();
      auto numCallThreeExitNodes = modRefSummary.GetCallExitNodes(test.CallThree()).Size();

      assert(numCallThreeEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallThreeExitNodes == pointsToGraph.NumMemoryNodes());

      auto numCallFourEntryNodes = modRefSummary.GetCallEntryNodes(test.CallFour()).Size();
      auto numCallFourExitNodes = modRefSummary.GetCallExitNodes(test.CallFour()).Size();

      assert(numCallFourEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallFourExitNodes == pointsToGraph.NumMemoryNodes());
    }
  };

  jlm::tests::IndirectCallTest1 test;
  //	jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  //	std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*PointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestGamma()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::GammaTest & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda).Size();
    auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda).Size();
    auto numGammaEntryNodes = modRefSummary.GetGammaEntryNodes(*test.gamma).Size();
    auto numGammaExitNodes = modRefSummary.GetGammaExitNodes(*test.gamma).Size();

    assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
    assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
    assert(numGammaEntryNodes == pointsToGraph.NumMemoryNodes());
    assert(numGammaExitNodes == pointsToGraph.NumMemoryNodes());
  };

  jlm::tests::GammaTest test;
  // jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*PointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestTheta()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::ThetaTest & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda).Size();
    auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda).Size();
    auto numThetaNodes = modRefSummary.GetThetaEntryExitNodes(*test.theta).Size();

    assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
    assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
    assert(numThetaNodes == pointsToGraph.NumMemoryNodes());
  };

  jlm::tests::ThetaTest test;
  //	jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  //	std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*PointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestDelta1()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::DeltaTest1 & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    /*
     * Validate function g
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_g).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_g).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
    }

    /*
     * Validate function h
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_h).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_h).Size();
      auto numCallEntryNodes = modRefSummary.GetCallEntryNodes(test.CallG()).Size();
      auto numCallExitNodes = modRefSummary.GetCallExitNodes(test.CallG()).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallExitNodes == pointsToGraph.NumMemoryNodes());
    }
  };

  jlm::tests::DeltaTest1 test;
  // jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*PointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestDelta2()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::DeltaTest2 & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    /*
     * Validate function f1
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_f1).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_f1).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
    }

    /*
     * Validate function f2
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_f2).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_f2).Size();
      auto numCallEntryNodes = modRefSummary.GetCallEntryNodes(test.CallF1()).Size();
      auto numCallExitNodes = modRefSummary.GetCallExitNodes(test.CallF1()).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallExitNodes == pointsToGraph.NumMemoryNodes());
    }
  };

  jlm::tests::DeltaTest2 test;
  //	jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*PointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestImports()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::ImportTest & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    /*
     * Validate function f1
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_f1).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_f1).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
    }

    /*
     * Validate function f2
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_f2).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_f2).Size();
      auto numCallEntryNodes = modRefSummary.GetCallEntryNodes(test.CallF1()).Size();
      auto numCallExitNodes = modRefSummary.GetCallExitNodes(test.CallF1()).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallExitNodes == pointsToGraph.NumMemoryNodes());
    }
  };

  jlm::tests::ImportTest test;
  //	jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*ptg);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestPhi1()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::PhiTest1 & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    /*
     * Validate function fib
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_fib).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_fib).Size();
      auto numGammaEntryNodes = modRefSummary.GetGammaEntryNodes(*test.gamma).Size();
      auto numGammaExitNodes = modRefSummary.GetGammaExitNodes(*test.gamma).Size();
      auto numCallFibm1EntryNodes = modRefSummary.GetCallEntryNodes(test.CallFibm1()).Size();
      auto numCallFibm1ExitNodes = modRefSummary.GetCallExitNodes(test.CallFibm1()).Size();
      auto numCallFibm2EntryNodes = modRefSummary.GetCallEntryNodes(test.CallFibm2()).Size();
      auto numCallFibm2ExitNodes = modRefSummary.GetCallExitNodes(test.CallFibm2()).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
      assert(numGammaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numGammaExitNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallFibm1EntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallFibm1ExitNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallFibm2EntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallFibm2ExitNodes == pointsToGraph.NumMemoryNodes());
    }

    /*
     * Validate function test
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(*test.lambda_test).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(*test.lambda_test).Size();
      auto numCallFibEntryNodes = modRefSummary.GetCallEntryNodes(test.CallFib()).Size();
      auto numCallFibExitNodes = modRefSummary.GetCallExitNodes(test.CallFib()).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallFibEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallFibExitNodes == pointsToGraph.NumMemoryNodes());
    }
  };

  jlm::tests::PhiTest1 test;
  //	jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*PointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestMemcpy()
{
  /*
   * Arrange
   */
  auto ValidateProvider = [](const jlm::tests::MemcpyTest & test,
                             const jlm::llvm::aa::ModRefSummary & modRefSummary,
                             const jlm::llvm::aa::PointsToGraph & pointsToGraph)
  {
    /*
     * Validate function f
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(test.LambdaF()).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(test.LambdaF()).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
    }

    /*
     * Validate function g
     */
    {
      auto numLambdaEntryNodes = modRefSummary.GetLambdaEntryNodes(test.LambdaG()).Size();
      auto numLambdaExitNodes = modRefSummary.GetLambdaExitNodes(test.LambdaG()).Size();
      auto numCallFEntryNodes = modRefSummary.GetCallEntryNodes(test.CallF()).Size();
      auto numCallFExitNodes = modRefSummary.GetCallExitNodes(test.CallF()).Size();

      auto numMemcpyDestNodes =
          modRefSummary.GetOutputNodes(*test.Memcpy().input(0)->origin()).Size();
      auto numMemcpySrcNodes =
          modRefSummary.GetOutputNodes(*test.Memcpy().input(1)->origin()).Size();

      assert(numLambdaEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numLambdaExitNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallFEntryNodes == pointsToGraph.NumMemoryNodes());
      assert(numCallFExitNodes == pointsToGraph.NumMemoryNodes());
      assert(numMemcpyDestNodes == 2);
      assert(numMemcpySrcNodes == 2);
    }
  };

  jlm::tests::MemcpyTest test;
  //	jlm::rvsdg::view(test.graph().GetRootRegion(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::llvm::aa::PointsToGraph::ToDot(*PointsToGraph);

  /*
   * Act
   */
  auto modRefSummary =
      jlm::llvm::aa::AgnosticModRefSummarizer::Create(test.module(), *pointsToGraph);

  /*
   * Assert
   */
  ValidateProvider(test, *modRefSummary, *pointsToGraph);
}

static void
TestStatistics()
{
  // Arrange
  jlm::tests::LoadTest1 test;
  auto pointsToGraph = RunSteensgaard(test.module());

  jlm::util::StatisticsCollectorSettings statisticsCollectorSettings(
      { jlm::util::Statistics::Id::AgnosticModRefSummarizer });
  jlm::util::StatisticsCollector statisticsCollector(statisticsCollectorSettings);

  // Act
  jlm::llvm::aa::AgnosticModRefSummarizer::Create(
      test.module(),
      *pointsToGraph,
      statisticsCollector);

  // Assert
  assert(statisticsCollector.NumCollectedStatistics() == 1);

  auto & statistics = dynamic_cast<const jlm::llvm::aa::AgnosticModRefSummarizer::Statistics &>(
      *statisticsCollector.CollectedStatistics().begin());

  assert(statistics.GetSourceFile() == test.module().SourceFileName());
  assert(statistics.NumPointsToGraphMemoryNodes() == 2);
  assert(statistics.GetTime() != 0);
}

static void
test()
{
  TestStore1();
  TestStore2();

  TestLoad1();
  TestLoad2();
  TestLoadFromUndef();

  TestCall1();
  TestCall2();
  TestIndirectCall();

  TestGamma();
  TestTheta();

  TestDelta1();
  TestDelta2();

  TestImports();

  TestPhi1();

  TestMemcpy();

  TestStatistics();
}

JLM_UNIT_TEST_REGISTER("jlm/llvm/opt/alias-analyses/AgnosticModRefSummarizerTests", test)
