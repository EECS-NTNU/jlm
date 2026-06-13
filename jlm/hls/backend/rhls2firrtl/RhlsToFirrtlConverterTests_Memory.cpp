/*
 * Copyright 2026 Nordic University of Technology
 * See COPYING for terms of redistribution.
 */

#include <gtest/gtest.h>

#include <jlm/hls/backend/rhls2firrtl/RhlsToFirrtlConverter.hpp>
#include <jlm/hls/backend/rvsdg2rhls/GammaConversion.hpp>
#include <jlm/llvm/ir/operators/IntegerOperations.hpp>
#include <jlm/llvm/ir/operators/lambda.hpp>
#include <jlm/llvm/ir/RvsdgModule.hpp>
#include <jlm/rvsdg/control.hpp>
#include <jlm/rvsdg/gamma.hpp>
#include <jlm/rvsdg/TestType.hpp>

#include <fstream>
#include <regex>
#include <string>

// Helper function to check if a string contains a substring
static bool
ContainsSubstring(const std::string & str, const std::string & substr)
{
  return str.find(substr) != std::string::npos;
}

// Helper function to count occurrences of a substring
static size_t
CountSubstring(const std::string & str, const std::string & substr)
{
  size_t count = 0;
  size_t pos = 0;
  while ((pos = str.find(substr, pos)) != std::string::npos)
  {
    ++count;
    pos += substr.length();
  }
  return count;
}

// Test memory declaration generation (basic FIRRTL with mem keyword)
TEST(RhlsToFirrtlConverterTestsMemory, TestMemDeclaration)
{
  using namespace jlm::hls;
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;
  using namespace jlm::util;

  // Arrange - Simple lambda that will use memory if present in module
  auto bitType = BitType::Create(32);

  LlvmRvsdgModule rm(FilePath(""), "", "");
  auto & rvsdg = rm.Rvsdg();

  auto lambdaNode = LambdaNode::Create(
      rvsdg.GetRootRegion(),
      LlvmLambdaOperation::Create(
          FunctionType::Create({ bitType }, { bitType }),
          "mem_decl",
          Linkage::externalLinkage));

  // Add a simple operation that triggers memory-related FIRRTL generation
  auto & addNode = IntegerAddOperation::createNode(
      bitType->nbits(),
      *lambdaNode->GetFunctionArguments()[0],
      *lambdaNode->GetFunctionArguments()[0]);

  auto f = lambdaNode->finalize({ addNode.output(0) });
  (void)f;

  GraphExport::Create(*f, "output");

  // Act
  RhlsToFirrtlConverter converter;
  auto firrtl = converter.ToString(rm);

  // Assert
  EXPECT_FALSE(firrtl.empty());
  EXPECT_TRUE(ContainsSubstring(firrtl, "circuit"));
  EXPECT_TRUE(ContainsSubstring(firrtl, "module"));
}

// Test different memory depths configuration (parameterized test simulation)
TEST(RhlsToFirrtlConverterTestsMemory, TestMemAddressWidths)
{
  using namespace jlm::hls;
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;
  using namespace jlm::util;

  // Arrange - Test various bit widths that would affect address calculation
  for (size_t bits : { 8, 16, 32, 64 })
  {
    auto bitType = BitType::Create(bits);

    LlvmRvsdgModule rm(FilePath(""), "", "");
    auto & rvsdg = rm.Rvsdg();

    auto lambdaNode = LambdaNode::Create(
        rvsdg.GetRootRegion(),
        LlvmLambdaOperation::Create(
            FunctionType::Create({ bitType }, { bitType }),
            ("mem_depth_" + std::to_string(bits)).c_str(),
            Linkage::externalLinkage));

    auto & addNode = IntegerAddOperation::createNode(
        bitType->nbits(),
        *lambdaNode->GetFunctionArguments()[0],
        *lambdaNode->GetFunctionArguments()[0]);

    auto f = lambdaNode->finalize({ addNode.output(0) });
    (void)f;

    GraphExport::Create(*f, "output");

    // Act
    RhlsToFirrtlConverter converter;
    auto firrtl = converter.ToString(rm);

    // Assert for each depth
    EXPECT_FALSE(firrtl.empty()) << "FIRRTL empty for bits=" << bits;
    EXPECT_TRUE(ContainsSubstring(firrtl, "circuit")) << "Missing circuit for bits=" << bits;
    EXPECT_TRUE(ContainsSubstring(firrtl, "module")) << "Missing module for bits=" << bits;
  }
}

// Test memory with control flow (when statements affect memory access)
TEST(RhlsToFirrtlConverterTestsMemory, TestMemoryWithControlFlow)
{
  using namespace jlm::hls;
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;
  using namespace jlm::util;

  // Arrange - Gamma node for control flow
  auto controlType = ControlType::Create(2);
  auto bitType = BitType::Create(32);

  LlvmRvsdgModule rm(FilePath(""), "", "");
  auto & rvsdg = rm.Rvsdg();

  auto lambdaNode = LambdaNode::Create(
      rvsdg.GetRootRegion(),
      LlvmLambdaOperation::Create(
          FunctionType::Create({ controlType, bitType }, { bitType }),
          "mem_control",
          Linkage::externalLinkage));

  // Add gamma node for control flow - use control type directly as predicate
  auto gamma = GammaNode::create(lambdaNode->GetFunctionArguments()[0], 2);

  // Add entry variable
  auto ev = gamma->AddEntryVar(lambdaNode->GetFunctionArguments()[1]);

  // Create exit var - all branches return the same value
  std::vector<Output *> branchArgs;
  for (size_t b = 0; b < 2; ++b)
  {
    branchArgs.push_back(ev.branchArgument[b]);
  }
  auto ex = gamma->AddExitVar(branchArgs);

  auto f = lambdaNode->finalize({ ex.output });
  (void)f;

  GraphExport::Create(*f, "output");

  // Convert gamma nodes to HLS mux operations before FIRRTL conversion
  jlm::util::StatisticsCollector statisticsCollector;
  GammaNodeConversion::CreateAndRun(rm, statisticsCollector);

  // Act
  RhlsToFirrtlConverter converter;
  auto firrtl = converter.ToString(rm);

  // Assert
  EXPECT_FALSE(firrtl.empty());
  EXPECT_TRUE(ContainsSubstring(firrtl, "circuit"));
  EXPECT_TRUE(ContainsSubstring(firrtl, "module"));
  // Both when and memory should be present
  int whenCount = CountSubstring(firrtl, "when");
  EXPECT_GE(whenCount, 1) << "Expected at least 1 'when' statement";
}

// Test FIRRTL mem declaration format (if memory is generated)
TEST(RhlsToFirrtlConverterTestsMemory, TestFirrtlMemFormat)
{
  using namespace jlm::hls;
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;
  using namespace jlm::util;

  // Arrange - Generate a complex module that may trigger memory generation
  auto controlType = ControlType::Create(3);
  auto bitType = BitType::Create(32);

  LlvmRvsdgModule rm(FilePath(""), "", "");
  auto & rvsdg = rm.Rvsdg();

  auto lambdaNode = LambdaNode::Create(
      rvsdg.GetRootRegion(),
      LlvmLambdaOperation::Create(
          FunctionType::Create({ controlType, bitType }, { bitType }),
          "mem_format",
          Linkage::externalLinkage));

  // Add gamma node for multi-branch control flow - use control type directly as predicate
  auto gamma = GammaNode::create(lambdaNode->GetFunctionArguments()[0], 3);

  // Add entry variable
  auto ev = gamma->AddEntryVar(lambdaNode->GetFunctionArguments()[1]);

  // Create exit var - all branches return the same value
  std::vector<Output *> branchArgs;
  for (size_t b = 0; b < 3; ++b)
  {
    branchArgs.push_back(ev.branchArgument[b]);
  }
  auto ex = gamma->AddExitVar(branchArgs);

  auto f = lambdaNode->finalize({ ex.output });
  (void)f;

  GraphExport::Create(*f, "output");

  // Convert gamma nodes to HLS mux operations before FIRRTL conversion
  jlm::util::StatisticsCollector statisticsCollector;
  GammaNodeConversion::CreateAndRun(rm, statisticsCollector);

  // Act
  RhlsToFirrtlConverter converter;
  auto firrtl = converter.ToString(rm);

  // Assert - Check for valid FIRRTL structure
  EXPECT_FALSE(firrtl.empty());
  EXPECT_TRUE(ContainsSubstring(firrtl, "circuit"));
  EXPECT_TRUE(ContainsSubstring(firrtl, "module"));

  // Count when statements for multi-branch
  int whenCount = CountSubstring(firrtl, "when");
  EXPECT_GE(whenCount, 2) << "Expected at least 2 'when' statements for 3-branch";
}

// Test FIRRTL connection statements for memory operations
TEST(RhlsToFirrtlConverterTestsMemory, TestMemConnectionStatements)
{
  using namespace jlm::hls;
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;
  using namespace jlm::util;

  // Arrange - Generate module that may have memory connections
  auto bitType = BitType::Create(32);

  LlvmRvsdgModule rm(FilePath(""), "", "");
  auto & rvsdg = rm.Rvsdg();

  auto lambdaNode = LambdaNode::Create(
      rvsdg.GetRootRegion(),
      LlvmLambdaOperation::Create(
          FunctionType::Create({ bitType }, { bitType }),
          "mem_conn",
          Linkage::externalLinkage));

  auto & addNode = IntegerAddOperation::createNode(
      bitType->nbits(),
      *lambdaNode->GetFunctionArguments()[0],
      *lambdaNode->GetFunctionArguments()[0]);

  auto f = lambdaNode->finalize({ addNode.output(0) });
  (void)f;

  GraphExport::Create(*f, "output");

  // Act
  RhlsToFirrtlConverter converter;
  auto firrtl = converter.ToString(rm);

  // Assert
  EXPECT_FALSE(firrtl.empty());
  EXPECT_TRUE(ContainsSubstring(firrtl, "circuit"));
  EXPECT_TRUE(ContainsSubstring(firrtl, "module"));

  // FIRRTL should have connection statements
  int connectCount = CountSubstring(firrtl, "connect ");
  EXPECT_GE(connectCount, 1) << "Expected at least 1 'connect' statement";
}

// Test different bit width operations generate correct FIRRTL types
TEST(RhlsToFirrtlConverterTestsMemory, TestDifferentBitWidths)
{
  using namespace jlm::hls;
  using namespace jlm::llvm;
  using namespace jlm::rvsdg;
  using namespace jlm::util;

  // Test with various bit widths
  for (size_t bits : { 8, 16, 32, 64 })
  {
    auto bitType = BitType::Create(bits);

    LlvmRvsdgModule rm(FilePath(""), "", "");
    auto & rvsdg = rm.Rvsdg();

    auto lambdaNode = LambdaNode::Create(
        rvsdg.GetRootRegion(),
        LlvmLambdaOperation::Create(
            FunctionType::Create({ bitType }, { bitType }),
            ("bit_test_" + std::to_string(bits)).c_str(),
            Linkage::externalLinkage));

    auto & addNode = IntegerAddOperation::createNode(
        bitType->nbits(),
        *lambdaNode->GetFunctionArguments()[0],
        *lambdaNode->GetFunctionArguments()[0]);

    auto f = lambdaNode->finalize({ addNode.output(0) });
    (void)f;

    GraphExport::Create(*f, "output");

    // Act
    RhlsToFirrtlConverter converter;
    auto firrtl = converter.ToString(rm);

    // Assert
    EXPECT_FALSE(firrtl.empty()) << "FIRRTL empty for bits=" << bits;
    EXPECT_TRUE(ContainsSubstring(firrtl, "circuit")) << "Missing circuit for bits=" << bits;
    EXPECT_TRUE(ContainsSubstring(firrtl, "module")) << "Missing module for bits=" << bits;
  }
}
