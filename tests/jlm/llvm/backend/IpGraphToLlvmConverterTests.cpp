/*
 * Copyright 2024, 2025 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <test-operation.hpp>
#include <test-registry.hpp>
#include <test-types.hpp>
#include <test-util.hpp>

#include <jlm/llvm/backend/IpGraphToLlvmConverter.hpp>
#include <jlm/llvm/ir/ipgraph-module.hpp>
#include <jlm/llvm/ir/operators.hpp>
#include <jlm/llvm/ir/operators/IntegerOperations.hpp>
#include <jlm/llvm/ir/print.hpp>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

static int
LoadConversion()
{
  using namespace jlm::llvm;

  // Arrange
  auto functionType = jlm::rvsdg::FunctionType::Create(
      { PointerType::Create(), MemoryStateType::Create() },
      { jlm::rvsdg::bittype::Create(64), MemoryStateType::Create() });

  ipgraph_module ipgModule(jlm::util::filepath(""), "", "");

  auto cfg = cfg::create(ipgModule);
  auto addressArgument =
      cfg->entry()->append_argument(argument::create("address", PointerType::Create()));
  auto memoryStateArgument =
      cfg->entry()->append_argument(argument::create("memoryState", MemoryStateType::Create()));

  auto basicBlock = basic_block::create(*cfg);
  size_t alignment = 4;
  auto loadTac = basicBlock->append_last(LoadNonVolatileOperation::Create(
      addressArgument,
      memoryStateArgument,
      jlm::rvsdg::bittype::Create(64),
      alignment));

  cfg->exit()->divert_inedges(basicBlock);
  basicBlock->add_outedge(cfg->exit());
  cfg->exit()->append_result(loadTac->result(0));
  cfg->exit()->append_result(loadTac->result(1));

  auto f = function_node::create(ipgModule.ipgraph(), "f", functionType, linkage::external_linkage);
  f->add_cfg(std::move(cfg));

  print(ipgModule, stdout);

  // Act
  llvm::LLVMContext ctx;
  auto llvmModule = IpGraphToLlvmConverter::CreateAndConvertModule(ipgModule, ctx);
  jlm::tests::print(*llvmModule);

  // Assert
  {
    auto llvmFunction = llvmModule->getFunction("f");
    auto & basicBlock = llvmFunction->back();
    auto & instruction = basicBlock.front();

    auto loadInstruction = ::llvm::dyn_cast<::llvm::LoadInst>(&instruction);
    assert(loadInstruction != nullptr);
    assert(loadInstruction->isVolatile() == false);
    assert(loadInstruction->getAlign().value() == alignment);
  }

  return 0;
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/backend/IpGraphToLlvmConverterTests-LoadConversion",
    LoadConversion)

static int
LoadVolatileConversion()
{
  using namespace jlm::llvm;

  // Arrange
  auto pointerType = PointerType::Create();
  auto ioStateType = IOStateType::Create();
  auto memoryStateType = MemoryStateType::Create();
  auto bit64Type = jlm::rvsdg::bittype::Create(64);
  auto functionType = jlm::rvsdg::FunctionType::Create(
      { PointerType::Create(), IOStateType::Create(), MemoryStateType::Create() },
      { jlm::rvsdg::bittype::Create(64), IOStateType::Create(), MemoryStateType::Create() });

  ipgraph_module ipgModule(jlm::util::filepath(""), "", "");

  auto cfg = cfg::create(ipgModule);
  auto addressArgument = cfg->entry()->append_argument(argument::create("address", pointerType));
  auto ioStateArgument = cfg->entry()->append_argument(argument::create("ioState", ioStateType));
  auto memoryStateArgument =
      cfg->entry()->append_argument(argument::create("memoryState", memoryStateType));

  auto basicBlock = basic_block::create(*cfg);
  size_t alignment = 4;
  auto loadTac = basicBlock->append_last(LoadVolatileOperation::Create(
      addressArgument,
      ioStateArgument,
      memoryStateArgument,
      bit64Type,
      alignment));

  cfg->exit()->divert_inedges(basicBlock);
  basicBlock->add_outedge(cfg->exit());
  cfg->exit()->append_result(loadTac->result(0));
  cfg->exit()->append_result(loadTac->result(1));
  cfg->exit()->append_result(loadTac->result(2));

  auto f = function_node::create(ipgModule.ipgraph(), "f", functionType, linkage::external_linkage);
  f->add_cfg(std::move(cfg));

  print(ipgModule, stdout);

  // Act
  llvm::LLVMContext ctx;
  auto llvmModule = IpGraphToLlvmConverter::CreateAndConvertModule(ipgModule, ctx);
  jlm::tests::print(*llvmModule);

  // Assert
  {
    auto llvmFunction = llvmModule->getFunction("f");
    auto & basicBlock = llvmFunction->back();
    auto & instruction = basicBlock.front();

    auto loadInstruction = ::llvm::dyn_cast<::llvm::LoadInst>(&instruction);
    assert(loadInstruction != nullptr);
    assert(loadInstruction->isVolatile() == true);
    assert(loadInstruction->getAlign().value() == alignment);
  }

  return 0;
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/backend/IpGraphToLlvmConverterTests-LoadVolatileConversion",
    LoadVolatileConversion)

static int
MemCpyConversion()
{
  using namespace jlm::llvm;

  // Arrange
  auto pointerType = PointerType::Create();
  auto memoryStateType = MemoryStateType::Create();
  auto bit64Type = jlm::rvsdg::bittype::Create(64);
  auto functionType = jlm::rvsdg::FunctionType::Create(
      { PointerType::Create(),
        PointerType::Create(),
        jlm::rvsdg::bittype::Create(64),
        MemoryStateType::Create() },
      { MemoryStateType::Create() });

  ipgraph_module ipgModule(jlm::util::filepath(""), "", "");

  auto cfg = cfg::create(ipgModule);
  auto destinationArgument =
      cfg->entry()->append_argument(argument::create("destination", pointerType));
  auto sourceArgument = cfg->entry()->append_argument(argument::create("source", pointerType));
  auto lengthArgument = cfg->entry()->append_argument(argument::create("length", bit64Type));
  auto memoryStateArgument =
      cfg->entry()->append_argument(argument::create("memoryState", memoryStateType));

  auto basicBlock = basic_block::create(*cfg);
  auto memCpyTac = basicBlock->append_last(MemCpyNonVolatileOperation::create(
      destinationArgument,
      sourceArgument,
      lengthArgument,
      { memoryStateArgument }));

  cfg->exit()->divert_inedges(basicBlock);
  basicBlock->add_outedge(cfg->exit());
  cfg->exit()->append_result(memCpyTac->result(0));

  auto f = function_node::create(ipgModule.ipgraph(), "f", functionType, linkage::external_linkage);
  f->add_cfg(std::move(cfg));

  print(ipgModule, stdout);

  // Act
  llvm::LLVMContext ctx;
  auto llvmModule = IpGraphToLlvmConverter::CreateAndConvertModule(ipgModule, ctx);
  jlm::tests::print(*llvmModule);

  // Assert
  {
    auto llvmFunction = llvmModule->getFunction("f");
    auto & basicBlock = llvmFunction->back();
    auto & instruction = basicBlock.front();

    auto memCpyInstruction = ::llvm::dyn_cast<::llvm::CallInst>(&instruction);
    assert(memCpyInstruction != nullptr);
    assert(memCpyInstruction->getIntrinsicID() == ::llvm::Intrinsic::memcpy);
    assert(memCpyInstruction->isVolatile() == false);
  }

  return 0;
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/backend/IpGraphToLlvmConverterTests-MemCpyConversion",
    MemCpyConversion)

static int
MemCpyVolatileConversion()
{
  using namespace jlm::llvm;

  // Arrange
  auto pointerType = PointerType::Create();
  auto ioStateType = IOStateType::Create();
  auto memoryStateType = MemoryStateType::Create();
  auto bit64Type = jlm::rvsdg::bittype::Create(64);
  auto functionType = jlm::rvsdg::FunctionType::Create(
      { PointerType::Create(),
        PointerType::Create(),
        jlm::rvsdg::bittype::Create(64),
        IOStateType::Create(),
        MemoryStateType::Create() },
      { IOStateType::Create(), MemoryStateType::Create() });

  ipgraph_module ipgModule(jlm::util::filepath(""), "", "");

  auto cfg = cfg::create(ipgModule);
  auto & destinationArgument =
      *cfg->entry()->append_argument(argument::create("destination", pointerType));
  auto & sourceArgument = *cfg->entry()->append_argument(argument::create("source", pointerType));
  auto & lengthArgument = *cfg->entry()->append_argument(argument::create("length", bit64Type));
  auto & ioStateArgument = *cfg->entry()->append_argument(argument::create("ioState", ioStateType));
  auto & memoryStateArgument =
      *cfg->entry()->append_argument(argument::create("memoryState", memoryStateType));

  auto basicBlock = basic_block::create(*cfg);
  auto memCpyTac = basicBlock->append_last(MemCpyVolatileOperation::CreateThreeAddressCode(
      destinationArgument,
      sourceArgument,
      lengthArgument,
      ioStateArgument,
      { &memoryStateArgument }));

  cfg->exit()->divert_inedges(basicBlock);
  basicBlock->add_outedge(cfg->exit());
  cfg->exit()->append_result(memCpyTac->result(0));
  cfg->exit()->append_result(memCpyTac->result(1));

  auto f = function_node::create(ipgModule.ipgraph(), "f", functionType, linkage::external_linkage);
  f->add_cfg(std::move(cfg));

  print(ipgModule, stdout);

  // Act
  llvm::LLVMContext ctx;
  auto llvmModule = IpGraphToLlvmConverter::CreateAndConvertModule(ipgModule, ctx);
  jlm::tests::print(*llvmModule);

  // Assert
  {
    auto llvmFunction = llvmModule->getFunction("f");
    auto & basicBlock = llvmFunction->back();
    auto & instruction = basicBlock.front();

    auto memCpyInstruction = ::llvm::dyn_cast<::llvm::CallInst>(&instruction);
    assert(memCpyInstruction != nullptr);
    assert(memCpyInstruction->getIntrinsicID() == ::llvm::Intrinsic::memcpy);
    assert(memCpyInstruction->isVolatile() == true);
  }

  return 0;
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/backend/IpGraphToLlvmConverterTests-MemCpyVolatileConversion",
    MemCpyVolatileConversion)

static int
StoreConversion()
{
  using namespace jlm::llvm;

  // Arrange
  auto pointerType = PointerType::Create();
  auto memoryStateType = MemoryStateType::Create();
  auto bit64Type = jlm::rvsdg::bittype::Create(64);
  auto functionType = jlm::rvsdg::FunctionType::Create(
      { PointerType::Create(), jlm::rvsdg::bittype::Create(64), MemoryStateType::Create() },
      { MemoryStateType::Create() });

  ipgraph_module ipgModule(jlm::util::filepath(""), "", "");

  auto cfg = cfg::create(ipgModule);
  auto addressArgument = cfg->entry()->append_argument(argument::create("address", pointerType));
  auto valueArgument = cfg->entry()->append_argument(argument::create("value", bit64Type));
  auto memoryStateArgument =
      cfg->entry()->append_argument(argument::create("memoryState", memoryStateType));

  auto basicBlock = basic_block::create(*cfg);
  size_t alignment = 4;
  auto storeTac = basicBlock->append_last(StoreNonVolatileOperation::Create(
      addressArgument,
      valueArgument,
      memoryStateArgument,
      alignment));

  cfg->exit()->divert_inedges(basicBlock);
  basicBlock->add_outedge(cfg->exit());
  cfg->exit()->append_result(storeTac->result(0));

  auto f = function_node::create(ipgModule.ipgraph(), "f", functionType, linkage::external_linkage);
  f->add_cfg(std::move(cfg));

  print(ipgModule, stdout);

  // Act
  llvm::LLVMContext ctx;
  auto llvmModule = IpGraphToLlvmConverter::CreateAndConvertModule(ipgModule, ctx);
  jlm::tests::print(*llvmModule);

  // Assert
  {
    auto llvmFunction = llvmModule->getFunction("f");
    auto & basicBlock = llvmFunction->back();
    auto & instruction = basicBlock.front();

    auto storeInstruction = ::llvm::dyn_cast<::llvm::StoreInst>(&instruction);
    assert(storeInstruction != nullptr);
    assert(storeInstruction->isVolatile() == false);
    assert(storeInstruction->getAlign().value() == alignment);
  }

  return 0;
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/backend/IpGraphToLlvmConverterTests-StoreConversion",
    StoreConversion)

static int
StoreVolatileConversion()
{
  using namespace jlm::llvm;

  // Arrange
  auto pointerType = PointerType::Create();
  auto ioStateType = IOStateType::Create();
  auto memoryStateType = MemoryStateType::Create();
  auto bit64Type = jlm::rvsdg::bittype::Create(64);
  auto functionType = jlm::rvsdg::FunctionType::Create(
      { PointerType::Create(),
        jlm::rvsdg::bittype::Create(64),
        IOStateType::Create(),
        MemoryStateType::Create() },
      { IOStateType::Create(), MemoryStateType::Create() });

  ipgraph_module ipgModule(jlm::util::filepath(""), "", "");

  auto cfg = cfg::create(ipgModule);
  auto addressArgument = cfg->entry()->append_argument(argument::create("address", pointerType));
  auto valueArgument = cfg->entry()->append_argument(argument::create("value", bit64Type));
  auto ioStateArgument = cfg->entry()->append_argument(argument::create("ioState", ioStateType));
  auto memoryStateArgument =
      cfg->entry()->append_argument(argument::create("memoryState", memoryStateType));

  auto basicBlock = basic_block::create(*cfg);
  size_t alignment = 4;
  auto storeTac = basicBlock->append_last(StoreVolatileOperation::Create(
      addressArgument,
      valueArgument,
      ioStateArgument,
      memoryStateArgument,
      alignment));

  cfg->exit()->divert_inedges(basicBlock);
  basicBlock->add_outedge(cfg->exit());
  cfg->exit()->append_result(storeTac->result(0));
  cfg->exit()->append_result(storeTac->result(1));

  auto f = function_node::create(ipgModule.ipgraph(), "f", functionType, linkage::external_linkage);
  f->add_cfg(std::move(cfg));

  print(ipgModule, stdout);

  // Act
  llvm::LLVMContext ctx;
  auto llvmModule = IpGraphToLlvmConverter::CreateAndConvertModule(ipgModule, ctx);
  jlm::tests::print(*llvmModule);

  // Assert
  {
    auto llvmFunction = llvmModule->getFunction("f");
    auto & basicBlock = llvmFunction->back();
    auto & instruction = basicBlock.front();

    auto storeInstruction = ::llvm::dyn_cast<::llvm::StoreInst>(&instruction);
    assert(storeInstruction != nullptr);
    assert(storeInstruction->isVolatile() == true);
    assert(storeInstruction->getAlign().value() == alignment);
  }

  return 0;
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/backend/IpGraphToLlvmConverterTests-StoreVolatileConversion",
    StoreVolatileConversion)

static int
IntegerConstant()
{
  const char * bs = "0100000000"
                    "0000000000"
                    "0000000000"
                    "0000000000"
                    "0000000000"
                    "0000000000"
                    "00001";

  using namespace jlm::llvm;

  auto ft = jlm::rvsdg::FunctionType::Create({}, { jlm::rvsdg::bittype::Create(65) });

  jlm::rvsdg::bitvalue_repr vr(bs);

  ipgraph_module im(jlm::util::filepath(""), "", "");

  auto cfg = cfg::create(im);
  auto bb = basic_block::create(*cfg);
  bb->append_last(tac::create(IntegerConstantOperation(vr), {}));
  auto c = bb->last()->result(0);

  cfg->exit()->divert_inedges(bb);
  bb->add_outedge(cfg->exit());
  cfg->exit()->append_result(c);

  auto f = function_node::create(im.ipgraph(), "f", ft, linkage::external_linkage);
  f->add_cfg(std::move(cfg));

  print(im, stdout);

  llvm::LLVMContext ctx;
  auto lm = IpGraphToLlvmConverter::CreateAndConvertModule(im, ctx);

  jlm::tests::print(*lm);

  return 0;
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/backend/IpGraphToLlvmConverterTests-IntegerConstant",
    IntegerConstant)

static int
Malloc()
{
  auto setup = []()
  {
    using namespace jlm::llvm;

    auto mt = MemoryStateType::Create();
    auto pt = PointerType::Create();
    auto im = ipgraph_module::create(jlm::util::filepath(""), "", "");

    auto cfg = cfg::create(*im);
    auto bb = basic_block::create(*cfg);
    cfg->exit()->divert_inedges(bb);
    bb->add_outedge(cfg->exit());

    auto size =
        cfg->entry()->append_argument(argument::create("size", jlm::rvsdg::bittype::Create(64)));

    bb->append_last(malloc_op::create(size));

    cfg->exit()->append_result(bb->last()->result(0));
    cfg->exit()->append_result(bb->last()->result(1));

    auto ft = jlm::rvsdg::FunctionType::Create(
        { jlm::rvsdg::bittype::Create(64) },
        { PointerType::Create(), MemoryStateType::Create() });
    auto f = function_node::create(im->ipgraph(), "f", ft, linkage::external_linkage);
    f->add_cfg(std::move(cfg));

    return im;
  };

  auto verify = [](const llvm::Module & m)
  {
    using namespace llvm;

    auto f = m.getFunction("f");
    auto & bb = f->getEntryBlock();

    assert(bb.sizeWithoutDebug() == 2);
    assert(bb.getFirstNonPHI()->getOpcode() == llvm::Instruction::Call);
    assert(bb.getTerminator()->getOpcode() == llvm::Instruction::Ret);
  };

  auto im = setup();
  print(*im, stdout);

  llvm::LLVMContext ctx;
  auto lm = jlm::llvm::IpGraphToLlvmConverter::CreateAndConvertModule(*im, ctx);
  jlm::tests::print(*lm);

  verify(*lm);

  return 0;
}

JLM_UNIT_TEST_REGISTER("jlm/llvm/backend/IpGraphToLlvmConverterTests-Malloc", Malloc)

static int
Free()
{
  auto setup = []()
  {
    using namespace jlm::llvm;

    auto iot = IOStateType::Create();
    auto mt = MemoryStateType::Create();
    auto pt = PointerType::Create();

    auto ipgmod = ipgraph_module::create(jlm::util::filepath(""), "", "");

    auto ft = jlm::rvsdg::FunctionType::Create(
        { PointerType::Create(), MemoryStateType::Create(), IOStateType::Create() },
        { MemoryStateType::Create(), IOStateType::Create() });
    auto f = function_node::create(ipgmod->ipgraph(), "f", ft, linkage::external_linkage);

    auto cfg = cfg::create(*ipgmod);
    auto arg0 = cfg->entry()->append_argument(argument::create("pointer", pt));
    auto arg1 = cfg->entry()->append_argument(argument::create("memstate", mt));
    auto arg2 = cfg->entry()->append_argument(argument::create("iostate", iot));

    auto bb = basic_block::create(*cfg);
    cfg->exit()->divert_inedges(bb);
    bb->add_outedge(cfg->exit());

    bb->append_last(FreeOperation::Create(arg0, { arg1 }, arg2));

    cfg->exit()->append_result(bb->last()->result(0));
    cfg->exit()->append_result(bb->last()->result(1));

    f->add_cfg(std::move(cfg));

    return ipgmod;
  };

  auto verify = [](const llvm::Module & module)
  {
    using namespace llvm;

    auto f = module.getFunction("f");
    auto & bb = f->getEntryBlock();

    assert(bb.sizeWithoutDebug() == 2);
    assert(bb.getFirstNonPHI()->getOpcode() == Instruction::Call);
    assert(bb.getTerminator()->getOpcode() == Instruction::Ret);
  };

  auto ipgmod = setup();
  print(*ipgmod, stdout);

  llvm::LLVMContext ctx;
  auto llvmmod = jlm::llvm::IpGraphToLlvmConverter::CreateAndConvertModule(*ipgmod, ctx);
  jlm::tests::print(*llvmmod);

  verify(*llvmmod);

  return 0;
}

JLM_UNIT_TEST_REGISTER("jlm/llvm/backend/IpGraphToLlvmConverterTests-Free", Free)

static int
IgnoreMemoryState()
{
  using namespace jlm::rvsdg;
  using namespace jlm::llvm;

  auto mt = MemoryStateType::Create();
  ipgraph_module m(jlm::util::filepath(""), "", "");

  std::unique_ptr<jlm::llvm::cfg> cfg(new jlm::llvm::cfg(m));
  auto bb = basic_block::create(*cfg);
  cfg->exit()->divert_inedges(bb);
  bb->add_outedge(cfg->exit());

  bb->append_last(UndefValueOperation::Create(mt, "s1"));
  auto s1 = bb->last()->result(0);

  cfg->exit()->append_result(s1);

  auto ft = FunctionType::Create({}, { mt });
  auto f = function_node::create(m.ipgraph(), "f", ft, linkage::external_linkage);
  f->add_cfg(std::move(cfg));

  llvm::LLVMContext ctx;
  IpGraphToLlvmConverter::CreateAndConvertModule(m, ctx);

  return 0;
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/backend/IpGraphToLlvmConverterTests-IgnoreMemoryState",
    IgnoreMemoryState)

static int
SelectWithState()
{
  using namespace jlm::llvm;

  auto vt = jlm::tests::valuetype::Create();
  auto pt = PointerType::Create();
  auto mt = MemoryStateType::Create();
  ipgraph_module m(jlm::util::filepath(""), "", "");

  std::unique_ptr<jlm::llvm::cfg> cfg(new jlm::llvm::cfg(m));
  auto bb = basic_block::create(*cfg);
  cfg->exit()->divert_inedges(bb);
  bb->add_outedge(cfg->exit());

  auto p = cfg->entry()->append_argument(argument::create("p", jlm::rvsdg::bittype::Create(1)));
  auto s1 = cfg->entry()->append_argument(argument::create("s1", mt));
  auto s2 = cfg->entry()->append_argument(argument::create("s2", mt));

  bb->append_last(SelectOperation::create(p, s1, s2));
  auto s3 = bb->last()->result(0);

  cfg->exit()->append_result(s3);
  cfg->exit()->append_result(s3);

  auto ft = jlm::rvsdg::FunctionType::Create(
      { jlm::rvsdg::bittype::Create(1), MemoryStateType::Create(), MemoryStateType::Create() },
      { MemoryStateType::Create(), MemoryStateType::Create() });
  auto f = function_node::create(m.ipgraph(), "f", ft, linkage::external_linkage);
  f->add_cfg(std::move(cfg));

  print(m, stdout);

  llvm::LLVMContext ctx;
  IpGraphToLlvmConverter::CreateAndConvertModule(m, ctx);

  return 0;
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/backend/IpGraphToLlvmConverterTests-SelectWithState",
    SelectWithState)

static int
TestAttributeKindConversion()
{
  typedef jlm::llvm::attribute::kind ak;

  int begin = static_cast<int>(ak::None);
  int end = static_cast<int>(ak::EndAttrKinds);
  for (int attributeKind = begin; attributeKind != end; attributeKind++)
  {
    jlm::llvm::IpGraphToLlvmConverter::ConvertAttributeKind(static_cast<ak>(attributeKind));
  }

  return 0;
}

JLM_UNIT_TEST_REGISTER(
    "jlm/llvm/backend/IpGraphToLlvmConverterTests-TestAttributeConversion",
    TestAttributeKindConversion)
