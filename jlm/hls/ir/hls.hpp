/*
 * Copyright 2021 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_HLS_IR_HLS_HPP
#define JLM_HLS_IR_HLS_HPP

#include <jlm/llvm/ir/operators/Store.hpp>
#include <jlm/llvm/ir/types.hpp>
#include <jlm/rvsdg/control.hpp>
#include <jlm/rvsdg/operation.hpp>
#include <jlm/rvsdg/structural-node.hpp>
#include <jlm/rvsdg/substitution.hpp>
#include <jlm/util/common.hpp>

#include <memory>
#include <utility>

namespace jlm::hls
{
/**
 * @return The size of a pointer in bits.
 */
[[nodiscard]] size_t
GetPointerSizeInBits();

int
JlmSize(const jlm::rvsdg::Type * type);

class BranchOperation final : public rvsdg::SimpleOperation
{
public:
  ~BranchOperation() noexcept override;

  BranchOperation(
      size_t nalternatives,
      const std::shared_ptr<const jlm::rvsdg::Type> & type,
      bool loop)
      : SimpleOperation(
            { rvsdg::ControlType::Create(nalternatives), type },
            { nalternatives, type }),
        loop(loop)
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    auto ot = dynamic_cast<const BranchOperation *>(&other);
    // check predicate and value
    return ot && ot->loop == loop && *ot->argument(0) == *argument(0)
        && *ot->result(0) == *result(0);
  }

  std::string
  debug_string() const override
  {
    return "HLS_BRANCH";
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<BranchOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(jlm::rvsdg::Output & predicate, jlm::rvsdg::Output & value, bool loop = false)
  {
    auto ctl = std::dynamic_pointer_cast<const rvsdg::ControlType>(predicate.Type());
    if (!ctl)
      throw util::Error("Predicate needs to be a control type.");

    return outputs(&rvsdg::CreateOpNode<BranchOperation>(
        { &predicate, &value },
        ctl->nalternatives(),
        value.Type(),
        loop));
  }

  bool loop; // only used for dot output
};

/**
 * Forks ensures 1-to-1 connections between producers and consumers, i.e., they handle fanout of
 * signals. Normal forks have a register inside to ensure that a token consumed on one output is not
 * repeated. The fork only creates an acknowledge on its single input once all outputs have been
 * consumed.
 *
 * CFORK (constant fork):
 * Handles the special case when the same constant is used as input for multiple nodes. It would be
 * possible to have a constant for each input, but deduplication replaces the constants with a
 * single constant fork. Since the input of the fork is always the same value and is always valid.
 * No handshaking is necessary and the outputs of the fork is always valid.
 */
class ForkOperation final : public rvsdg::SimpleOperation
{
public:
  ~ForkOperation() noexcept override;

  /**
   * Create a fork operation that is not a constant fork.
   *
   * /param nalternatives Number of outputs.
   * /param value The signal type, which is the same for the input and all outputs.
   */
  ForkOperation(size_t nalternatives, const std::shared_ptr<const jlm::rvsdg::Type> & type)
      : SimpleOperation({ type }, { nalternatives, type })
  {}

  /**
   * Create a fork operation.
   *
   * /param nalternatives Number of outputs.
   * /param value The signal type, which is the same for the input and all outputs.
   * /param isConstant If true, the fork is a constant fork.
   */
  ForkOperation(
      size_t nalternatives,
      const std::shared_ptr<const jlm::rvsdg::Type> & type,
      bool isConstant)
      : SimpleOperation({ type }, { nalternatives, type }),
        IsConstant_(isConstant)
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    const auto forkOp = dynamic_cast<const ForkOperation *>(&other);
    // check predicate and value
    return forkOp && *forkOp->argument(0) == *argument(0) && forkOp->nresults() == nresults()
        && forkOp->IsConstant() == IsConstant_;
  }

  /**
   * Debug string for the fork operation.
   * /return HLS_CFORK if the fork is a constant fork, else HLS_FORK.
   */
  std::string
  debug_string() const override
  {
    return IsConstant() ? "HLS_CFORK" : "HLS_FORK";
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<ForkOperation>(*this);
  }

  /**
   * Create a fork operation with a single input and multiple outputs.
   *
   * /param nalternatives Number of outputs.
   * /param value The signal type, which is the same for the input and all outputs.
   * /param isConstant If true, the fork is a constant fork.
   *
   * /return A vector of outputs.
   */
  static std::vector<jlm::rvsdg::Output *>
  create(size_t nalternatives, jlm::rvsdg::Output & value, bool isConstant = false)
  {
    return outputs(
        &rvsdg::CreateOpNode<ForkOperation>({ &value }, nalternatives, value.Type(), isConstant));
  }

  /**
   * Create a ForkOperation node.
   *
   * @param numResults Number of outputs.
   * @param operand The node's operand
   * @param isConstant If true, the ForkOperation is a constant fork.
   *
   * @return A ForkOperation node.
   */
  static rvsdg::Node &
  CreateNode(const size_t numResults, rvsdg::Output & operand, const bool isConstant = false)
  {
    return rvsdg::CreateOpNode<ForkOperation>({ &operand }, numResults, operand.Type(), isConstant);
  }

  /**
   * Check if a fork is a constant fork (CFORK).
   *
   * /return True if the fork is a constant fork, i.e., the input of the fork is a constant, else
   * false.
   */
  [[nodiscard]] bool
  IsConstant() const noexcept
  {
    return IsConstant_;
  }

private:
  bool IsConstant_ = false;
};

class MuxOperation final : public rvsdg::SimpleOperation
{
public:
  ~MuxOperation() noexcept override;

  MuxOperation(
      size_t nalternatives,
      const std::shared_ptr<const jlm::rvsdg::Type> & type,
      bool discarding,
      bool loop)
      : SimpleOperation(create_typevector(nalternatives, type), { type }),
        discarding(discarding),
        loop(loop)
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    const auto ot = dynamic_cast<const MuxOperation *>(&other);
    // check predicate and value
    return ot && *ot->argument(0) == *argument(0) && *ot->result(0) == *result(0)
        && ot->discarding == discarding;
  }

  std::string
  debug_string() const override
  {
    return discarding ? "HLS_DMUX" : "HLS_NDMUX";
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<MuxOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(
      jlm::rvsdg::Output & predicate,
      const std::vector<jlm::rvsdg::Output *> & alternatives,
      bool discarding,
      bool loop = false)
  {
    if (alternatives.empty())
      throw util::Error("Insufficient number of operands.");
    auto ctl = std::dynamic_pointer_cast<const rvsdg::ControlType>(predicate.Type());
    if (!ctl)
      throw util::Error("Predicate needs to be a control type.");
    if (alternatives.size() != ctl->nalternatives())
      throw util::Error("Alternatives and predicate do not match.");

    auto operands = std::vector<jlm::rvsdg::Output *>();
    operands.push_back(&predicate);
    operands.insert(operands.end(), alternatives.begin(), alternatives.end());
    return outputs(&rvsdg::CreateOpNode<MuxOperation>(
        operands,
        alternatives.size(),
        alternatives.front()->Type(),
        discarding,
        loop));
  }

  bool discarding;
  bool loop; // used only for dot output
private:
  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  create_typevector(size_t nalternatives, std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    auto vec =
        std::vector<std::shared_ptr<const jlm::rvsdg::Type>>(nalternatives + 1, std::move(type));
    vec[0] = rvsdg::ControlType::Create(nalternatives);
    return vec;
  }
};

class SinkOperation final : public rvsdg::SimpleOperation
{
public:
  ~SinkOperation() noexcept override;

  explicit SinkOperation(const std::shared_ptr<const jlm::rvsdg::Type> & type)
      : SimpleOperation({ type }, {})
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    const auto ot = dynamic_cast<const SinkOperation *>(&other);
    return ot && *ot->argument(0) == *argument(0);
  }

  std::string
  debug_string() const override
  {
    return "HLS_SINK";
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<SinkOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(jlm::rvsdg::Output & value)
  {
    return outputs(&rvsdg::CreateOpNode<SinkOperation>({ &value }, value.Type()));
  }
};

class PredicateBufferOperation final : public rvsdg::SimpleOperation
{
public:
  ~PredicateBufferOperation() noexcept override;

  explicit PredicateBufferOperation(const std::shared_ptr<const rvsdg::ControlType> & type)
      : SimpleOperation({ type }, { type })
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    const auto ot = dynamic_cast<const PredicateBufferOperation *>(&other);
    return ot && *ot->result(0) == *result(0);
  }

  std::string
  debug_string() const override
  {
    return "HLS_PRED_BUF";
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<PredicateBufferOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(jlm::rvsdg::Output & predicate)
  {
    auto ctl = std::dynamic_pointer_cast<const rvsdg::ControlType>(predicate.Type());
    if (!ctl)
      throw util::Error("Predicate needs to be a control type.");

    return outputs(&rvsdg::CreateOpNode<PredicateBufferOperation>({ &predicate }, ctl));
  }
};

class LoopConstantBufferOperation final : public rvsdg::SimpleOperation
{
public:
  ~LoopConstantBufferOperation() noexcept override;

  LoopConstantBufferOperation(
      const std::shared_ptr<const rvsdg::ControlType> & ctltype,
      const std::shared_ptr<const jlm::rvsdg::Type> & type)
      : SimpleOperation({ ctltype, type }, { type })
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    const auto ot = dynamic_cast<const LoopConstantBufferOperation *>(&other);
    return ot && *ot->result(0) == *result(0) && *ot->argument(0) == *argument(0);
  }

  std::string
  debug_string() const override
  {
    return "HLS_LOOP_CONST_BUF";
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<LoopConstantBufferOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(jlm::rvsdg::Output & predicate, jlm::rvsdg::Output & value)
  {
    auto ctl = std::dynamic_pointer_cast<const rvsdg::ControlType>(predicate.Type());
    if (!ctl)
      throw util::Error("Predicate needs to be a control type.");

    return outputs(&rvsdg::CreateOpNode<LoopConstantBufferOperation>(
        { &predicate, &value },
        ctl,
        value.Type()));
  }
};

class BufferOperation final : public rvsdg::SimpleOperation
{
public:
  ~BufferOperation() noexcept override;

  BufferOperation(
      const std::shared_ptr<const jlm::rvsdg::Type> & type,
      size_t capacity,
      bool pass_through)
      : SimpleOperation({ type }, { type }),
        Capacity_(capacity),
        IsPassThrough_(pass_through)
  {}

  [[nodiscard]] std::size_t
  Capacity() const noexcept
  {
    return Capacity_;
  }

  [[nodiscard]] bool
  IsPassThrough() const noexcept
  {
    return IsPassThrough_;
  }

  bool
  operator==(const Operation & other) const noexcept override
  {
    const auto ot = dynamic_cast<const BufferOperation *>(&other);
    return ot && ot->Capacity() == Capacity() && ot->IsPassThrough() == IsPassThrough()
        && *ot->result(0) == *result(0);
  }

  [[nodiscard]] std::string
  debug_string() const override
  {
    return util::strfmt("HLS_BUF_", (IsPassThrough() ? "P_" : ""), Capacity());
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<BufferOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(jlm::rvsdg::Output & value, size_t capacity, bool pass_through = false)
  {
    return outputs(
        &rvsdg::CreateOpNode<BufferOperation>({ &value }, value.Type(), capacity, pass_through));
  }

private:
  std::size_t Capacity_;
  bool IsPassThrough_;
};

class TriggerType final : public rvsdg::StateType
{
public:
  ~TriggerType() noexcept override;

  TriggerType() = default;

  std::string
  debug_string() const override
  {
    return "trigger";
  };

  bool
  operator==(const Type & other) const noexcept override
  {
    return jlm::rvsdg::is<TriggerType>(other);
  };

  [[nodiscard]] std::size_t
  ComputeHash() const noexcept override;

  static std::shared_ptr<const TriggerType>
  Create();
};

class TriggerOperation final : public rvsdg::SimpleOperation
{
public:
  ~TriggerOperation() noexcept override;

  explicit TriggerOperation(const std::shared_ptr<const rvsdg::Type> & type)
      : SimpleOperation({ TriggerType::Create(), type }, { type })
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    const auto ot = dynamic_cast<const TriggerOperation *>(&other);
    // check predicate and value
    return ot && *ot->argument(1) == *argument(1) && *ot->result(0) == *result(0);
  }

  std::string
  debug_string() const override
  {
    return "HLS_TRIGGER";
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<TriggerOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(jlm::rvsdg::Output & tg, jlm::rvsdg::Output & value)
  {
    if (!rvsdg::is<TriggerType>(tg.Type()))
      throw util::Error("Trigger needs to be a TriggerType.");

    return outputs(&rvsdg::CreateOpNode<TriggerOperation>({ &tg, &value }, value.Type()));
  }
};

class PrintOperation final : public rvsdg::SimpleOperation
{
  size_t _id;

public:
  ~PrintOperation() noexcept override;

  explicit PrintOperation(const std::shared_ptr<const rvsdg::Type> & type)
      : SimpleOperation({ type }, { type })
  {
    static size_t common_id{ 0 };
    _id = common_id++;
  }

  bool
  operator==(const Operation &) const noexcept override
  {
    // print nodes are intentionally distinct
    return false;
  }

  std::string
  debug_string() const override
  {
    return util::strfmt("HLS_PRINT_", _id);
  }

  size_t
  id() const
  {
    return _id;
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<PrintOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(jlm::rvsdg::Output & value)
  {
    return outputs(&rvsdg::CreateOpNode<PrintOperation>({ &value }, value.Type()));
  }
};

class LoopOperation final : public rvsdg::StructuralOperation
{
public:
  ~LoopOperation() noexcept override;

  std::string
  debug_string() const override
  {
    return "HLS_LOOP";
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<LoopOperation>(*this);
  }
};

class backedge_argument;
class backedge_result;
class LoopNode;

/**
 * Represents the entry argument for the HLS loop.
 */
class EntryArgument : public rvsdg::RegionArgument
{
  friend LoopNode;

public:
  ~EntryArgument() noexcept override;

private:
  EntryArgument(
      rvsdg::Region & region,
      rvsdg::StructuralInput & input,
      const std::shared_ptr<const rvsdg::Type> type)
      : rvsdg::RegionArgument(&region, &input, std::move(type))
  {}

public:
  EntryArgument &
  Copy(rvsdg::Region & region, rvsdg::StructuralInput * input) override;

  // FIXME: This should not be public, but we currently still have some transformations that use
  // this one. Make it eventually private.
  static EntryArgument &
  Create(
      rvsdg::Region & region,
      rvsdg::StructuralInput & input,
      const std::shared_ptr<const rvsdg::Type> type)
  {
    auto argument = new EntryArgument(region, input, std::move(type));
    region.append_argument(argument);
    return *argument;
  }
};

class backedge_argument : public rvsdg::RegionArgument
{
  friend LoopNode;
  friend backedge_result;

public:
  ~backedge_argument() override = default;

  backedge_result *
  result()
  {
    return result_;
  }

  backedge_argument &
  Copy(rvsdg::Region & region, rvsdg::StructuralInput * input) override;

private:
  backedge_argument(rvsdg::Region * region, const std::shared_ptr<const jlm::rvsdg::Type> & type)
      : rvsdg::RegionArgument(region, nullptr, type),
        result_(nullptr)
  {}

  static backedge_argument *
  create(rvsdg::Region * region, std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    auto argument = new backedge_argument(region, std::move(type));
    region->append_argument(argument);
    return argument;
  }

  backedge_result * result_;
};

class backedge_result : public rvsdg::RegionResult
{
  friend LoopNode;
  friend backedge_argument;

public:
  ~backedge_result() override = default;

  backedge_argument *
  argument() const
  {
    return argument_;
  }

  backedge_result &
  Copy(rvsdg::Output & origin, rvsdg::StructuralOutput * output) override;

private:
  backedge_result(jlm::rvsdg::Output * origin)
      : rvsdg::RegionResult(origin->region(), origin, nullptr, origin->Type()),
        argument_(nullptr)
  {}

  static backedge_result *
  create(jlm::rvsdg::Output * origin)
  {
    auto result = new backedge_result(origin);
    origin->region()->append_result(result);
    return result;
  }

  backedge_argument * argument_;
};

/**
 * Represents the exit result of the HLS loop.
 */
class ExitResult final : public rvsdg::RegionResult
{
  friend LoopNode;

public:
  ~ExitResult() noexcept override;

private:
  ExitResult(rvsdg::Output & origin, rvsdg::StructuralOutput & output);

public:
  ExitResult &
  Copy(rvsdg::Output & origin, rvsdg::StructuralOutput * output) override;

  // FIXME: This should not be public, but we currently still have some transformations that use
  // this one. Make it eventually private.
  static ExitResult &
  Create(rvsdg::Output & origin, rvsdg::StructuralOutput & output)
  {
    auto result = new ExitResult(origin, output);
    origin.region()->append_result(result);
    return *result;
  }
};

class LoopNode final : public rvsdg::StructuralNode
{
public:
  ~LoopNode() noexcept override = default;

private:
  explicit LoopNode(rvsdg::Region * parent)
      : StructuralNode(parent, 1)
  {}

public:
  [[nodiscard]] const rvsdg::Operation &
  GetOperation() const noexcept override;

  static LoopNode *
  create(rvsdg::Region * parent, bool init = true);

  rvsdg::Region *
  subregion() const noexcept
  {
    return StructuralNode::subregion(0);
  }

  [[nodiscard]] rvsdg::RegionResult *
  predicate() const noexcept
  {
    auto result = subregion()->result(0);
    JLM_ASSERT(rvsdg::is<const rvsdg::ControlType>(result->Type()));
    return result;
  }

  rvsdg::Output &
  GetPredicateBuffer() const noexcept
  {
    return *PredicateBuffer_;
  }

  void
  set_predicate(jlm::rvsdg::Output * p);

  backedge_argument *
  add_backedge(std::shared_ptr<const jlm::rvsdg::Type> type);

  rvsdg::StructuralOutput *
  AddLoopVar(jlm::rvsdg::Output * origin, jlm::rvsdg::Output ** buffer = nullptr);

  jlm::rvsdg::Output *
  add_loopconst(jlm::rvsdg::Output * origin);

  LoopNode *
  copy(rvsdg::Region * region, rvsdg::SubstitutionMap & smap) const override;

private:
  rvsdg::Output * PredicateBuffer_{};
};

class BundleType final : public rvsdg::ValueType
{
public:
  ~BundleType() noexcept override;

  explicit BundleType(
      const std::vector<std::pair<std::string, std::shared_ptr<const Type>>> elements)
      : elements_(std::move(elements))
  {}

  BundleType(const BundleType &) = default;

  BundleType(BundleType &&) = delete;

  BundleType &
  operator=(const BundleType &) = delete;

  BundleType &
  operator=(BundleType &&) = delete;

  bool
  operator==(const jlm::rvsdg::Type & other) const noexcept override
  {
    auto type = dynamic_cast<const BundleType *>(&other);
    // TODO: better comparison?
    if (!type || type->elements_.size() != elements_.size())
    {
      return false;
    }
    for (size_t i = 0; i < elements_.size(); ++i)
    {
      if (type->elements_.at(i).first != elements_.at(i).first
          || *type->elements_.at(i).second != *elements_.at(i).second)
      {
        return false;
      }
    }
    return true;
  };

  [[nodiscard]] std::size_t
  ComputeHash() const noexcept override;

  std::shared_ptr<const jlm::rvsdg::Type>
  get_element_type(std::string element) const
  {
    for (size_t i = 0; i < elements_.size(); ++i)
    {
      if (elements_.at(i).first == element)
      {
        return elements_.at(i).second;
      }
    }
    // TODO: do something different?
    return {};
  }

  [[nodiscard]] std::string
  debug_string() const override
  {
    return "bundle";
  };

  //        private:
  // TODO: fix memory leak
  const std::vector<std::pair<std::string, std::shared_ptr<const jlm::rvsdg::Type>>> elements_;
};

std::shared_ptr<const BundleType>
get_mem_req_type(std::shared_ptr<const rvsdg::ValueType> elementType, bool write);

std::shared_ptr<const BundleType>
get_mem_res_type(std::shared_ptr<const jlm::rvsdg::ValueType> dataType);

class LoadOperation final : public rvsdg::SimpleOperation
{
public:
  ~LoadOperation() noexcept override;

  LoadOperation(const std::shared_ptr<const rvsdg::ValueType> & pointeeType, size_t numStates)
      : SimpleOperation(
            CreateInTypes(pointeeType, numStates),
            CreateOutTypes(pointeeType, numStates))
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    auto ot = dynamic_cast<const LoadOperation *>(&other);
    // check predicate and value
    return ot && *ot->argument(1) == *argument(1) && ot->narguments() == narguments();
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateInTypes(std::shared_ptr<const rvsdg::ValueType> pointeeType, size_t numStates)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(
        1,
        llvm::PointerType::Create()); // addr
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> states(
        numStates,
        llvm::MemoryStateType::Create());
    types.insert(types.end(), states.begin(), states.end());
    types.emplace_back(std::move(pointeeType)); // result
    return types;
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateOutTypes(std::shared_ptr<const rvsdg::ValueType> pointeeType, size_t numStates)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(1, std::move(pointeeType));
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> states(
        numStates,
        llvm::MemoryStateType::Create());
    types.insert(types.end(), states.begin(), states.end());
    types.emplace_back(llvm::PointerType::Create()); // addr
    return types;
  }

  std::string
  debug_string() const override
  {
    return "HLS_LOAD_" + argument(narguments() - 1)->debug_string();
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<LoadOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(
      jlm::rvsdg::Output & addr,
      const std::vector<jlm::rvsdg::Output *> & states,
      jlm::rvsdg::Output & load_result)
  {
    std::vector<jlm::rvsdg::Output *> inputs;
    inputs.push_back(&addr);
    inputs.insert(inputs.end(), states.begin(), states.end());
    inputs.push_back(&load_result);
    return outputs(&rvsdg::CreateOpNode<LoadOperation>(
        inputs,
        std::dynamic_pointer_cast<const rvsdg::ValueType>(load_result.Type()),
        states.size()));
  }

  [[nodiscard]] const llvm::PointerType &
  GetPointerType() const noexcept
  {
    return *util::AssertedCast<const llvm::PointerType>(argument(0).get());
  }

  [[nodiscard]] std::shared_ptr<const rvsdg::ValueType>
  GetLoadedType() const noexcept
  {
    return std::dynamic_pointer_cast<const rvsdg::ValueType>(result(0));
  }
};

class AddressQueueOperation final : public rvsdg::SimpleOperation
{
public:
  ~AddressQueueOperation() noexcept override;

  AddressQueueOperation(
      const std::shared_ptr<const llvm::PointerType> & pointerType,
      size_t capacity,
      bool combinatorial)
      : SimpleOperation(CreateInTypes(pointerType), CreateOutTypes(pointerType)),
        combinatorial(combinatorial),
        capacity(capacity)
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    auto ot = dynamic_cast<const AddressQueueOperation *>(&other);
    // check predicate and value
    return ot && *ot->argument(1) == *argument(1) && ot->narguments() == narguments();
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateInTypes(std::shared_ptr<const llvm::PointerType> pointerType)
  {
    // check, enq
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(2, std::move(pointerType));
    types.emplace_back(llvm::MemoryStateType::Create()); // deq
    return types;
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateOutTypes(std::shared_ptr<const llvm::PointerType> pointerType)
  {
    return { std::move(pointerType) };
  }

  std::string
  debug_string() const override
  {
    if (combinatorial)
    {
      return "HLS_ADDR_QUEUE_COMB_" + argument(narguments() - 1)->debug_string();
    }
    return "HLS_ADDR_QUEUE_" + argument(narguments() - 1)->debug_string();
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<AddressQueueOperation>(*this);
  }

  static jlm::rvsdg::Output *
  create(
      jlm::rvsdg::Output & check,
      jlm::rvsdg::Output & enq,
      jlm::rvsdg::Output & deq,
      bool combinatorial,
      size_t capacity = 10)
  {
    return rvsdg::CreateOpNode<AddressQueueOperation>(
               { &check, &enq, &deq },
               std::dynamic_pointer_cast<const llvm::PointerType>(check.Type()),
               capacity,
               combinatorial)
        .output(0);
  }

  bool combinatorial;
  size_t capacity;
};

class StateGateOperation final : public rvsdg::SimpleOperation
{
public:
  ~StateGateOperation() noexcept override;

  StateGateOperation(const std::shared_ptr<const rvsdg::Type> & type, const size_t numStates)
      : SimpleOperation(CreateInOutTypes(type, numStates), CreateInOutTypes(type, numStates))
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    auto ot = dynamic_cast<const StateGateOperation *>(&other);
    // check predicate and value
    return ot && *ot->argument(1) == *argument(1) && ot->narguments() == narguments();
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateInOutTypes(const std::shared_ptr<const jlm::rvsdg::Type> & type, size_t numStates)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(1, type);
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> states(
        numStates,
        llvm::MemoryStateType::Create());
    types.insert(types.end(), states.begin(), states.end());
    return types;
  }

  std::string
  debug_string() const override
  {
    return "HLS_STATE_GATE_" + argument(narguments() - 1)->debug_string();
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<StateGateOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(jlm::rvsdg::Output & addr, const std::vector<jlm::rvsdg::Output *> & states)
  {
    std::vector<jlm::rvsdg::Output *> inputs;
    inputs.push_back(&addr);
    inputs.insert(inputs.end(), states.begin(), states.end());
    return outputs(&rvsdg::CreateOpNode<StateGateOperation>(inputs, addr.Type(), states.size()));
  }
};

class DecoupledLoadOperation final : public rvsdg::SimpleOperation
{
public:
  ~DecoupledLoadOperation() noexcept override;

  DecoupledLoadOperation(
      const std::shared_ptr<const rvsdg::ValueType> & pointeeType,
      size_t capacity)
      : SimpleOperation(CreateInTypes(pointeeType), CreateOutTypes(pointeeType)),
        capacity(capacity)
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    auto ot = dynamic_cast<const DecoupledLoadOperation *>(&other);
    // check predicate and value
    return ot && *ot->argument(1) == *argument(1) && ot->narguments() == narguments();
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateInTypes(std::shared_ptr<const rvsdg::ValueType> pointeeType)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(1, llvm::PointerType::Create());
    types.emplace_back(std::move(pointeeType)); // result
    return types;
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateOutTypes(std::shared_ptr<const rvsdg::ValueType> pointeeType)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(1, std::move(pointeeType));
    types.emplace_back(llvm::PointerType::Create()); // addr
    return types;
  }

  std::string
  debug_string() const override
  {
    return "HLS_DEC_LOAD_" + std::to_string(capacity) + "_"
         + argument(narguments() - 1)->debug_string();
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<DecoupledLoadOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(jlm::rvsdg::Output & addr, jlm::rvsdg::Output & load_result, size_t capacity)
  {
    std::vector<jlm::rvsdg::Output *> inputs;
    inputs.push_back(&addr);
    inputs.push_back(&load_result);
    JLM_ASSERT(capacity >= 1);
    return outputs(&rvsdg::CreateOpNode<DecoupledLoadOperation>(
        inputs,
        std::dynamic_pointer_cast<const rvsdg::ValueType>(load_result.Type()),
        capacity));
  }

  [[nodiscard]] const llvm::PointerType &
  GetPointerType() const noexcept
  {
    return *util::AssertedCast<const llvm::PointerType>(argument(0).get());
  }

  [[nodiscard]] std::shared_ptr<const rvsdg::ValueType>
  GetLoadedType() const noexcept
  {
    return std::dynamic_pointer_cast<const rvsdg::ValueType>(result(0));
  }

  size_t capacity;
};

class MemoryResponseOperation final : public rvsdg::SimpleOperation
{
public:
  ~MemoryResponseOperation() noexcept override;

  explicit MemoryResponseOperation(
      const std::vector<std::shared_ptr<const rvsdg::Type>> & output_types,
      int in_width)
      : SimpleOperation(CreateInTypes(in_width), CreateOutTypes(output_types))
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    auto ot = dynamic_cast<const MemoryResponseOperation *>(&other);
    // check predicate and value
    return ot && *ot->argument(1) == *argument(1) && ot->narguments() == narguments();
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateInTypes(int in_width)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types;
    types.emplace_back(get_mem_res_type(jlm::rvsdg::bittype::Create(in_width)));
    return types;
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateOutTypes(const std::vector<std::shared_ptr<const rvsdg::Type>> & output_types)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types;
    types.reserve(output_types.size());
    for (auto outputType : output_types)
    {
      types.emplace_back(outputType);
    }
    return types;
  }

  std::string
  debug_string() const override
  {
    return "HLS_MEM_RESP";
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<MemoryResponseOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(
      rvsdg::Output & result,
      const std::vector<std::shared_ptr<const rvsdg::Type>> & output_types,
      int in_width)
  {
    return outputs(
        &rvsdg::CreateOpNode<MemoryResponseOperation>({ &result }, output_types, in_width));
  }
};

class MemoryRequestOperation final : public rvsdg::SimpleOperation
{
public:
  ~MemoryRequestOperation() noexcept override = default;

  MemoryRequestOperation(
      const std::vector<std::shared_ptr<const rvsdg::ValueType>> & load_types,
      const std::vector<std::shared_ptr<const rvsdg::ValueType>> & store_types)
      : SimpleOperation(
            CreateInTypes(load_types, store_types),
            CreateOutTypes(load_types, store_types))
  {
    for (auto loadType : load_types)
    {
      LoadTypes_.emplace_back(loadType);
    }
    for (auto storeType : store_types)
    {
      StoreTypes_.emplace_back(storeType);
    }
  }

  MemoryRequestOperation(const MemoryRequestOperation & other) = default;

  bool
  operator==(const Operation & other) const noexcept override
  {
    auto ot = dynamic_cast<const MemoryRequestOperation *>(&other);
    // check predicate and value
    return ot && ot->narguments() == narguments()
        && (ot->narguments() == 0 || (*ot->argument(1) == *argument(1)))
        && ot->narguments() == narguments();
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateInTypes(
      const std::vector<std::shared_ptr<const rvsdg::ValueType>> & load_types,
      const std::vector<std::shared_ptr<const rvsdg::ValueType>> & store_types)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types;
    for (size_t i = 0; i < load_types.size(); i++)
    {
      types.emplace_back(llvm::PointerType::Create()); // addr
    }
    for (auto storeType : store_types)
    {
      types.emplace_back(llvm::PointerType::Create()); // addr
      types.emplace_back(storeType);                   // data
    }
    return types;
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateOutTypes(
      const std::vector<std::shared_ptr<const rvsdg::ValueType>> & load_types,
      const std::vector<std::shared_ptr<const rvsdg::ValueType>> & store_types)
  {
    int max_width = 0;
    for (auto tp : load_types)
    {
      auto sz = JlmSize(tp.get());
      max_width = sz > max_width ? sz : max_width;
    }
    for (auto tp : store_types)
    {
      auto sz = JlmSize(tp.get());
      max_width = sz > max_width ? sz : max_width;
    }
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types;
    types.emplace_back(
        get_mem_req_type(jlm::rvsdg::bittype::Create(max_width), !store_types.empty()));
    return types;
  }

  std::string
  debug_string() const override
  {
    return "HLS_MEM_REQ";
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<MemoryRequestOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(
      const std::vector<jlm::rvsdg::Output *> & load_operands,
      const std::vector<std::shared_ptr<const rvsdg::ValueType>> & loadTypes,
      const std::vector<jlm::rvsdg::Output *> & store_operands,
      rvsdg::Region *)
  {
    // Stores have both addr and data operand
    // But we are only interested in the data operand type
    JLM_ASSERT(store_operands.size() % 2 == 0);
    std::vector<std::shared_ptr<const rvsdg::ValueType>> storeTypes;
    for (size_t i = 1; i < store_operands.size(); i += 2)
    {
      storeTypes.push_back(
          std::dynamic_pointer_cast<const rvsdg::ValueType>(store_operands[i]->Type()));
    }
    std::vector operands(load_operands);
    operands.insert(operands.end(), store_operands.begin(), store_operands.end());
    return outputs(&rvsdg::CreateOpNode<MemoryRequestOperation>(operands, loadTypes, storeTypes));
  }

  size_t
  get_nloads() const
  {
    return LoadTypes_.size();
  }

  const std::vector<std::shared_ptr<const rvsdg::Type>> *
  GetLoadTypes() const
  {
    return &LoadTypes_;
  }

  const std::vector<std::shared_ptr<const rvsdg::Type>> *
  GetStoreTypes() const
  {
    return &StoreTypes_;
  }

private:
  std::vector<std::shared_ptr<const rvsdg::Type>> LoadTypes_;
  std::vector<std::shared_ptr<const rvsdg::Type>> StoreTypes_;
};

class StoreOperation final : public rvsdg::SimpleOperation
{
public:
  ~StoreOperation() noexcept override;

  StoreOperation(const std::shared_ptr<const rvsdg::ValueType> & pointeeType, size_t numStates)
      : SimpleOperation(
            CreateInTypes(pointeeType, numStates),
            CreateOutTypes(pointeeType, numStates))
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    auto ot = dynamic_cast<const StoreOperation *>(&other);
    // check predicate and value
    return ot && *ot->argument(1) == *argument(1) && ot->narguments() == narguments();
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateInTypes(const std::shared_ptr<const rvsdg::ValueType> & pointeeType, size_t numStates)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(
        { llvm::PointerType::Create(), pointeeType });
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> states(
        numStates + 1,
        llvm::MemoryStateType::Create());
    types.insert(types.end(), states.begin(), states.end());
    return types;
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateOutTypes(const std::shared_ptr<const rvsdg::ValueType> & pointeeType, size_t numStates)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(
        numStates,
        llvm::MemoryStateType::Create());
    types.emplace_back(llvm::PointerType::Create()); // addr
    types.emplace_back(pointeeType);                 // data
    return types;
  }

  std::string
  debug_string() const override
  {
    return "HLS_STORE_" + argument(narguments() - 1)->debug_string();
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<StoreOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(
      jlm::rvsdg::Output & addr,
      jlm::rvsdg::Output & value,
      const std::vector<jlm::rvsdg::Output *> & states,
      jlm::rvsdg::Output & resp)
  {
    std::vector<jlm::rvsdg::Output *> inputs;
    inputs.push_back(&addr);
    inputs.push_back(&value);
    inputs.insert(inputs.end(), states.begin(), states.end());
    inputs.push_back(&resp);
    return outputs(&rvsdg::CreateOpNode<StoreOperation>(
        inputs,
        std::dynamic_pointer_cast<const rvsdg::ValueType>(value.Type()),
        states.size()));
  }

  [[nodiscard]] const llvm::PointerType &
  GetPointerType() const noexcept
  {
    return *util::AssertedCast<const llvm::PointerType>(argument(0).get());
  }

  [[nodiscard]] const rvsdg::ValueType &
  GetStoredType() const noexcept
  {
    return *util::AssertedCast<const rvsdg::ValueType>(argument(1).get());
  }
};

class LocalMemoryOperation final : public rvsdg::SimpleOperation
{
public:
  ~LocalMemoryOperation() noexcept override;

  explicit LocalMemoryOperation(std::shared_ptr<const llvm::ArrayType> at)
      : SimpleOperation({}, CreateOutTypes(std::move(at)))
  {}

  bool
  operator==(const Operation &) const noexcept override
  {
    return false;
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateOutTypes(std::shared_ptr<const llvm::ArrayType> at)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(2, std::move(at));
    return types;
  }

  std::string
  debug_string() const override
  {
    return "HLS_LOCAL_MEM_" + result(0)->debug_string();
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<LocalMemoryOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(std::shared_ptr<const llvm::ArrayType> at, rvsdg::Region * region)
  {
    return outputs(&rvsdg::CreateOpNode<LocalMemoryOperation>(*region, std::move(at)));
  }
};

class LocalMemoryResponseOperation final : public rvsdg::SimpleOperation
{
public:
  ~LocalMemoryResponseOperation() noexcept override;

  LocalMemoryResponseOperation(const std::shared_ptr<const llvm::ArrayType> & at, size_t resp_count)
      : SimpleOperation({ at }, CreateOutTypes(at, resp_count))
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    auto ot = dynamic_cast<const LocalMemoryResponseOperation *>(&other);
    // check predicate and value
    return ot && *ot->argument(1) == *argument(1) && ot->narguments() == narguments();
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateOutTypes(const std::shared_ptr<const jlm::llvm::ArrayType> & at, size_t resp_count)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(resp_count, at->GetElementType());
    return types;
  }

  std::string
  debug_string() const override
  {
    return "HLS_LOCAL_MEM_RESP";
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<LocalMemoryResponseOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(jlm::rvsdg::Output & mem, size_t resp_count)
  {
    return outputs(&rvsdg::CreateOpNode<LocalMemoryResponseOperation>(
        { &mem },
        std::dynamic_pointer_cast<const llvm::ArrayType>(mem.Type()),
        resp_count));
  }
};

class LocalLoadOperation final : public rvsdg::SimpleOperation
{
public:
  ~LocalLoadOperation() noexcept override;

  LocalLoadOperation(
      const std::shared_ptr<const jlm::rvsdg::ValueType> & valuetype,
      size_t numStates)
      : SimpleOperation(CreateInTypes(valuetype, numStates), CreateOutTypes(valuetype, numStates))
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    auto ot = dynamic_cast<const LocalLoadOperation *>(&other);
    // check predicate and value
    return ot && *ot->argument(1) == *argument(1) && ot->narguments() == narguments();
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateInTypes(const std::shared_ptr<const jlm::rvsdg::ValueType> & valuetype, size_t numStates)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(1, jlm::rvsdg::bittype::Create(64));
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> states(
        numStates,
        llvm::MemoryStateType::Create());
    types.insert(types.end(), states.begin(), states.end());
    types.emplace_back(valuetype); // result
    return types;
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateOutTypes(const std::shared_ptr<const jlm::rvsdg::ValueType> & valuetype, size_t numStates)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(1, valuetype);
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> states(
        numStates,
        llvm::MemoryStateType::Create());
    types.insert(types.end(), states.begin(), states.end());
    types.emplace_back(jlm::rvsdg::bittype::Create(64)); // addr
    return types;
  }

  std::string
  debug_string() const override
  {
    return "HLS_LOCAL_LOAD_" + argument(narguments() - 1)->debug_string();
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<LocalLoadOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(
      jlm::rvsdg::Output & index,
      const std::vector<jlm::rvsdg::Output *> & states,
      jlm::rvsdg::Output & load_result)
  {
    std::vector<jlm::rvsdg::Output *> inputs;
    inputs.push_back(&index);
    inputs.insert(inputs.end(), states.begin(), states.end());
    inputs.push_back(&load_result);
    return outputs(&rvsdg::CreateOpNode<LocalLoadOperation>(
        inputs,
        std::dynamic_pointer_cast<const jlm::rvsdg::ValueType>(load_result.Type()),
        states.size()));
  }

  [[nodiscard]] std::shared_ptr<const rvsdg::ValueType>
  GetLoadedType() const noexcept
  {
    return std::dynamic_pointer_cast<const rvsdg::ValueType>(result(0));
  }
};

class LocalStoreOperation final : public rvsdg::SimpleOperation
{
public:
  ~LocalStoreOperation() noexcept override;

  LocalStoreOperation(
      const std::shared_ptr<const jlm::rvsdg::ValueType> & valuetype,
      size_t numStates)
      : SimpleOperation(CreateInTypes(valuetype, numStates), CreateOutTypes(valuetype, numStates))
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    auto ot = dynamic_cast<const LocalStoreOperation *>(&other);
    // check predicate and value
    return ot && *ot->argument(1) == *argument(1) && ot->narguments() == narguments();
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateInTypes(const std::shared_ptr<const jlm::rvsdg::ValueType> & valuetype, size_t numStates)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(
        { jlm::rvsdg::bittype::Create(64), valuetype });
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> states(
        numStates,
        llvm::MemoryStateType::Create());
    types.insert(types.end(), states.begin(), states.end());
    return types;
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateOutTypes(const std::shared_ptr<const jlm::rvsdg::ValueType> & valuetype, size_t numStates)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(
        numStates,
        llvm::MemoryStateType::Create());
    types.emplace_back(jlm::rvsdg::bittype::Create(64)); // addr
    types.emplace_back(valuetype);                       // data
    return types;
  }

  std::string
  debug_string() const override
  {
    return "HLS_LOCAL_STORE_" + argument(narguments() - 1)->debug_string();
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<LocalStoreOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(
      jlm::rvsdg::Output & index,
      jlm::rvsdg::Output & value,
      const std::vector<jlm::rvsdg::Output *> & states)
  {
    std::vector<jlm::rvsdg::Output *> inputs;
    inputs.push_back(&index);
    inputs.push_back(&value);
    inputs.insert(inputs.end(), states.begin(), states.end());
    return outputs(&rvsdg::CreateOpNode<LocalStoreOperation>(
        inputs,
        std::dynamic_pointer_cast<const jlm::rvsdg::ValueType>(value.Type()),
        states.size()));
  }

  [[nodiscard]] const jlm::rvsdg::ValueType &
  GetStoredType() const noexcept
  {
    return *util::AssertedCast<const jlm::rvsdg::ValueType>(argument(1).get());
  }
};

class LocalMemoryRequestOperation final : public rvsdg::SimpleOperation
{
public:
  ~LocalMemoryRequestOperation() noexcept override;

  LocalMemoryRequestOperation(
      const std::shared_ptr<const llvm::ArrayType> & at,
      size_t load_cnt,
      size_t store_cnt)
      : SimpleOperation(CreateInTypes(at, load_cnt, store_cnt), {})
  {}

  bool
  operator==(const Operation & other) const noexcept override
  {
    auto ot = dynamic_cast<const LocalMemoryRequestOperation *>(&other);
    // check predicate and value
    return ot && ot->narguments() == narguments()
        && (ot->narguments() == 0 || (*ot->argument(1) == *argument(1)))
        && ot->narguments() == narguments();
  }

  static std::vector<std::shared_ptr<const jlm::rvsdg::Type>>
  CreateInTypes(
      const std::shared_ptr<const llvm::ArrayType> & at,
      size_t load_cnt,
      size_t store_cnt)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types(1, at);
    for (size_t i = 0; i < load_cnt; ++i)
    {
      types.emplace_back(jlm::rvsdg::bittype::Create(64)); // addr
    }
    for (size_t i = 0; i < store_cnt; ++i)
    {
      types.emplace_back(jlm::rvsdg::bittype::Create(64)); // addr
      types.emplace_back(at->GetElementType());            // data
    }
    return types;
  }

  std::string
  debug_string() const override
  {
    return "HLS_LOCAL_MEM_REQ";
  }

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override
  {
    return std::make_unique<LocalMemoryRequestOperation>(*this);
  }

  static std::vector<jlm::rvsdg::Output *>
  create(
      jlm::rvsdg::Output & mem,
      const std::vector<jlm::rvsdg::Output *> & load_operands,
      const std::vector<jlm::rvsdg::Output *> & store_operands)
  {
    JLM_ASSERT(store_operands.size() % 2 == 0);
    std::vector operands(1, &mem);
    operands.insert(operands.end(), load_operands.begin(), load_operands.end());
    operands.insert(operands.end(), store_operands.begin(), store_operands.end());
    return outputs(&rvsdg::CreateOpNode<LocalMemoryRequestOperation>(
        operands,
        std::dynamic_pointer_cast<const llvm::ArrayType>(mem.Type()),
        load_operands.size(),
        store_operands.size() / 2));
  }
};

}
#endif // JLM_HLS_IR_HLS_HPP
