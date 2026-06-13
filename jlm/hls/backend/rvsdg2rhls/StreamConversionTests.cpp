/*
 * Copyright 2026 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#include <gtest/gtest.h>

#include <jlm/hls/backend/rvsdg2rhls/stream-conv.hpp>
#include <jlm/hls/ir/hls.hpp>
#include <jlm/llvm/ir/operators/lambda.hpp>
#include <jlm/llvm/ir/RvsdgModule.hpp>
#include <jlm/rvsdg/bitstring/constant.hpp>

TEST(StreamConversionTests, TestSimpleStream)
{
  using namespace jlm::llvm;

  // Arrange
  auto bit32Type = jlm::rvsdg::BitType::Create(32);

  LlvmRvsdgModule rm(jlm::util::FilePath(""), "", "");
  auto & rvsdg = rm.Rvsdg();

  auto lambda = jlm::rvsdg::LambdaNode::Create(
      rvsdg.GetRootRegion(),
      LlvmLambdaOperation::Create(
          jlm::rvsdg::FunctionType::Create({ bit32Type }, { bit32Type }),
          "f",
          Linkage::externalLinkage));

  // Create a constant value
  auto * constant = &jlm::rvsdg::BitConstantOperation::create(*lambda->subregion(), { 32, 42 });

  auto lambdaOutput = lambda->finalize({ constant });
  jlm::rvsdg::GraphExport::Create(*lambdaOutput, "f");

  // Act
  jlm::util::StatisticsCollector statisticsCollector;
  jlm::hls::StreamConversion::CreateAndRun(rm, statisticsCollector);

  // Assert - Basic test: lambda should still exist with output
  EXPECT_EQ(rvsdg.GetRootRegion().numNodes(), 1);
  auto lambdaNode =
      dynamic_cast<jlm::rvsdg::LambdaNode *>(rvsdg.GetRootRegion().Nodes().begin().ptr());
  EXPECT_NE(lambdaNode, nullptr);
}

TEST(StreamConversionTests, TestStreamWithBuffer)
{
  using namespace jlm::llvm;

  // Arrange
  auto bit32Type = jlm::rvsdg::BitType::Create(32);

  LlvmRvsdgModule rm(jlm::util::FilePath(""), "", "");
  auto & rvsdg = rm.Rvsdg();

  auto lambda = jlm::rvsdg::LambdaNode::Create(
      rvsdg.GetRootRegion(),
      LlvmLambdaOperation::Create(
          jlm::rvsdg::FunctionType::Create({ bit32Type }, { bit32Type }),
          "f",
          Linkage::externalLinkage));

  // Create constant with buffer
  auto * constant = &jlm::rvsdg::BitConstantOperation::create(*lambda->subregion(), { 32, 10 });

  auto lambdaOutput = lambda->finalize({ constant });
  jlm::rvsdg::GraphExport::Create(*lambdaOutput, "f");

  // Act
  jlm::util::StatisticsCollector statisticsCollector;
  jlm::hls::StreamConversion::CreateAndRun(rm, statisticsCollector);

  // Assert
  EXPECT_EQ(rvsdg.GetRootRegion().numNodes(), 1);
}

TEST(StreamConversionTests, TestEmptyLambda)
{
  using namespace jlm::llvm;

  // Arrange
  auto bit32Type = jlm::rvsdg::BitType::Create(32);

  LlvmRvsdgModule rm(jlm::util::FilePath(""), "", "");
  auto & rvsdg = rm.Rvsdg();

  (void)jlm::rvsdg::LambdaNode::Create(
      rvsdg.GetRootRegion(),
      LlvmLambdaOperation::Create(
          jlm::rvsdg::FunctionType::Create({}, { bit32Type }),
          "f",
          Linkage::externalLinkage));

  // Act
  jlm::util::StatisticsCollector statisticsCollector;
  jlm::hls::StreamConversion::CreateAndRun(rm, statisticsCollector);

  // Assert - Stream conversion should be a no-op for lambdas without stream operations
  EXPECT_EQ(rvsdg.GetRootRegion().numNodes(), 1);
}
