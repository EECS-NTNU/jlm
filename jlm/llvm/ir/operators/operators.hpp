/*
 * Copyright 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_LLVM_IR_OPERATORS_OPERATORS_HPP
#define JLM_LLVM_IR_OPERATORS_OPERATORS_HPP

#include <jlm/llvm/ir/ipgraph-module.hpp>
#include <jlm/llvm/ir/tac.hpp>
#include <jlm/llvm/ir/types.hpp>
#include <jlm/rvsdg/binary.hpp>
#include <jlm/rvsdg/bitstring/type.hpp>
#include <jlm/rvsdg/control.hpp>
#include <jlm/rvsdg/simple-node.hpp>
#include <jlm/rvsdg/type.hpp>
#include <jlm/rvsdg/unary.hpp>

#include <llvm/ADT/APFloat.h>

namespace jlm::llvm
{

/**
 * Operation that picks its value based on which node branched to the current basic block.
 * All SsaPhiOperations must be at the top of their basic blocks.
 *
 * Each operand corresponds to an incoming basic block,
 * and the list of incoming nodes must include every predecessor in the cfg exactly once.
 */
class SsaPhiOperation final : public rvsdg::SimpleOperation
{
public:
  ~SsaPhiOperation() noexcept override;

  SsaPhiOperation(
      std::vector<cfg_node *> incomingNodes,
      const std::shared_ptr<const jlm::rvsdg::Type> & type)
      : SimpleOperation({ incomingNodes.size(), type }, { type }),
        IncomingNodes_(std::move(incomingNodes))
  {}

  SsaPhiOperation(const SsaPhiOperation &) = default;

  SsaPhiOperation &
  operator=(const SsaPhiOperation &) = delete;

  SsaPhiOperation &
  operator=(SsaPhiOperation &&) = delete;

  bool
  operator==(const Operation & other) const noexcept override;

  std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  const rvsdg::Type &
  type() const noexcept
  {
    return *result(0);
  }

  const std::shared_ptr<const rvsdg::Type> &
  Type() const noexcept
  {
    return result(0);
  }

  cfg_node *
  GetIncomingNode(size_t n) const noexcept
  {
    JLM_ASSERT(n < narguments());
    return IncomingNodes_[n];
  }

  static std::unique_ptr<llvm::tac>
  create(
      const std::vector<std::pair<const variable *, cfg_node *>> & arguments,
      std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    std::vector<cfg_node *> basicBlocks;
    std::vector<const variable *> operands;
    for (const auto & argument : arguments)
    {
      basicBlocks.push_back(argument.second);
      operands.push_back(argument.first);
    }

    const SsaPhiOperation phi(std::move(basicBlocks), std::move(type));
    return tac::create(phi, operands);
  }

private:
  std::vector<cfg_node *> IncomingNodes_;
};

class AssignmentOperation final : public rvsdg::SimpleOperation
{
public:
  ~AssignmentOperation() noexcept override;

  explicit AssignmentOperation(const std::shared_ptr<const rvsdg::Type> & type)
      : SimpleOperation({ type, type }, {})
  {}

  AssignmentOperation(const AssignmentOperation &) = default;

  AssignmentOperation(AssignmentOperation &&) = default;

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  static std::unique_ptr<llvm::tac>
  create(const variable * rhs, const variable * lhs)
  {
    if (rhs->type() != lhs->type())
      throw jlm::util::error("LHS and RHS of assignment must have same type.");

    return tac::create(AssignmentOperation(rhs->Type()), { lhs, rhs });
  }
};

class SelectOperation final : public rvsdg::SimpleOperation
{
public:
  ~SelectOperation() noexcept override;

  explicit SelectOperation(const std::shared_ptr<const rvsdg::Type> & type)
      : SimpleOperation({ rvsdg::bittype::Create(1), type, type }, { type })
  {}

  bool
  operator==(const Operation & other) const noexcept override;

  std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  [[nodiscard]] const jlm::rvsdg::Type &
  type() const noexcept
  {
    return *result(0);
  }

  [[nodiscard]] const std::shared_ptr<const jlm::rvsdg::Type> &
  Type() const noexcept
  {
    return result(0);
  }

  static std::unique_ptr<llvm::tac>
  create(const llvm::variable * p, const llvm::variable * t, const llvm::variable * f)
  {
    const SelectOperation op(t->Type());
    return tac::create(op, { p, t, f });
  }
};

class VectorSelectOperation final : public rvsdg::SimpleOperation
{
public:
  ~VectorSelectOperation() noexcept override;

private:
  VectorSelectOperation(
      const std::shared_ptr<const VectorType> & pt,
      const std::shared_ptr<const VectorType> & vt)
      : SimpleOperation({ pt, vt, vt }, { vt })
  {}

public:
  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  [[nodiscard]] const rvsdg::Type &
  type() const noexcept
  {
    return *result(0);
  }

  [[nodiscard]] const std::shared_ptr<const rvsdg::Type> &
  Type() const noexcept
  {
    return result(0);
  }

  size_t
  size() const noexcept
  {
    return dynamic_cast<const VectorType *>(&type())->size();
  }

  static std::unique_ptr<llvm::tac>
  create(const variable * p, const variable * t, const variable * f)
  {
    if (is<FixedVectorType>(p->type()) && is<FixedVectorType>(t->type()))
      return createVectorSelectTac<FixedVectorType>(p, t, f);

    if (is<ScalableVectorType>(p->type()) && is<ScalableVectorType>(t->type()))
      return createVectorSelectTac<ScalableVectorType>(p, t, f);

    throw jlm::util::error("Expected vector types as operands.");
  }

private:
  template<typename T>
  static std::unique_ptr<tac>
  createVectorSelectTac(const variable * p, const variable * t, const variable * f)
  {
    auto fvt = static_cast<const T *>(&t->type());
    auto pt = T::Create(jlm::rvsdg::bittype::Create(1), fvt->size());
    auto vt = T::Create(fvt->Type(), fvt->size());
    const VectorSelectOperation op(pt, vt);
    return tac::create(op, { p, t, f });
  }
};

class FloatingPointToUnsignedIntegerOperation final : public rvsdg::UnaryOperation
{
public:
  ~FloatingPointToUnsignedIntegerOperation() noexcept override;

  FloatingPointToUnsignedIntegerOperation(
      const fpsize size,
      std::shared_ptr<const rvsdg::bittype> type)
      : UnaryOperation(FloatingPointType::Create(size), std::move(type))
  {}

  FloatingPointToUnsignedIntegerOperation(
      std::shared_ptr<const FloatingPointType> fpt,
      std::shared_ptr<const jlm::rvsdg::bittype> type)
      : UnaryOperation(std::move(fpt), std::move(type))
  {}

  FloatingPointToUnsignedIntegerOperation(
      std::shared_ptr<const jlm::rvsdg::Type> srctype,
      std::shared_ptr<const jlm::rvsdg::Type> dsttype)
      : UnaryOperation(srctype, dsttype)
  {
    auto st = dynamic_cast<const FloatingPointType *>(srctype.get());
    if (!st)
      throw jlm::util::error("expected floating point type.");

    auto dt = dynamic_cast<const jlm::rvsdg::bittype *>(dsttype.get());
    if (!dt)
      throw jlm::util::error("expected bitstring type.");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  jlm::rvsdg::unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::output * output) const noexcept override;

  jlm::rvsdg::output *
  reduce_operand(jlm::rvsdg::unop_reduction_path_t path, jlm::rvsdg::output * output)
      const override;

  static std::unique_ptr<llvm::tac>
  create(const variable * operand, const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto st = std::dynamic_pointer_cast<const FloatingPointType>(operand->Type());
    if (!st)
      throw jlm::util::error("expected floating point type.");

    auto dt = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(type);
    if (!dt)
      throw jlm::util::error("expected bitstring type.");

    const FloatingPointToUnsignedIntegerOperation op(std::move(st), std::move(dt));
    return tac::create(op, { operand });
  }
};

class FloatingPointToSignedIntegerOperation final : public rvsdg::UnaryOperation
{
public:
  ~FloatingPointToSignedIntegerOperation() noexcept override;

  FloatingPointToSignedIntegerOperation(
      const fpsize size,
      std::shared_ptr<const jlm::rvsdg::bittype> type)
      : UnaryOperation(FloatingPointType::Create(size), std::move(type))
  {}

  FloatingPointToSignedIntegerOperation(
      std::shared_ptr<const FloatingPointType> fpt,
      std::shared_ptr<const jlm::rvsdg::bittype> type)
      : UnaryOperation(std::move(fpt), std::move(type))
  {}

  FloatingPointToSignedIntegerOperation(
      std::shared_ptr<const jlm::rvsdg::Type> srctype,
      std::shared_ptr<const jlm::rvsdg::Type> dsttype)
      : UnaryOperation(srctype, dsttype)
  {
    auto st = dynamic_cast<const FloatingPointType *>(srctype.get());
    if (!st)
      throw jlm::util::error("expected floating point type.");

    auto dt = dynamic_cast<const jlm::rvsdg::bittype *>(dsttype.get());
    if (!dt)
      throw jlm::util::error("expected bitstring type.");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  jlm::rvsdg::unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::output * output) const noexcept override;

  jlm::rvsdg::output *
  reduce_operand(jlm::rvsdg::unop_reduction_path_t path, jlm::rvsdg::output * output)
      const override;

  static std::unique_ptr<llvm::tac>
  create(const variable * operand, const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto st = std::dynamic_pointer_cast<const FloatingPointType>(operand->Type());
    if (!st)
      throw jlm::util::error("expected floating point type.");

    auto dt = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(type);
    if (!dt)
      throw jlm::util::error("expected bitstring type.");

    FloatingPointToSignedIntegerOperation op(std::move(st), std::move(dt));
    return tac::create(op, { operand });
  }
};

/* ctl2bits operator */

class ctl2bits_op final : public rvsdg::SimpleOperation
{
public:
  virtual ~ctl2bits_op() noexcept;

  inline ctl2bits_op(
      std::shared_ptr<const rvsdg::ControlType> srctype,
      std::shared_ptr<const jlm::rvsdg::bittype> dsttype)
      : SimpleOperation({ std::move(srctype) }, { std::move(dsttype) })
  {}

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  static std::unique_ptr<llvm::tac>
  create(const variable * operand, const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto st = std::dynamic_pointer_cast<const rvsdg::ControlType>(operand->Type());
    if (!st)
      throw jlm::util::error("expected control type.");

    auto dt = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(type);
    if (!dt)
      throw jlm::util::error("expected bitstring type.");

    ctl2bits_op op(std::move(st), std::move(dt));
    return tac::create(op, { operand });
  }
};

class BranchOperation final : public rvsdg::SimpleOperation
{
public:
  ~BranchOperation() noexcept override;

  explicit BranchOperation(std::shared_ptr<const rvsdg::ControlType> type)
      : SimpleOperation({ std::move(type) }, {})
  {}

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  inline size_t
  nalternatives() const noexcept
  {
    return std::static_pointer_cast<const rvsdg::ControlType>(argument(0))->nalternatives();
  }

  static std::unique_ptr<llvm::tac>
  create(size_t nalternatives, const variable * operand)
  {
    const BranchOperation op(rvsdg::ControlType::Create(nalternatives));
    return tac::create(op, { operand });
  }
};

/** \brief ConstantPointerNullOperation class
 *
 * This operator is the Jlm equivalent of LLVM's ConstantPointerNull constant.
 */
class ConstantPointerNullOperation final : public rvsdg::SimpleOperation
{
public:
  ~ConstantPointerNullOperation() noexcept override;

  explicit ConstantPointerNullOperation(std::shared_ptr<const PointerType> pointerType)
      : SimpleOperation({}, { std::move(pointerType) })
  {}

  bool
  operator==(const Operation & other) const noexcept override;

  [[nodiscard]] std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  [[nodiscard]] const PointerType &
  GetPointerType() const noexcept
  {
    return *util::AssertedCast<const PointerType>(result(0).get());
  }

  static std::unique_ptr<llvm::tac>
  Create(std::shared_ptr<const rvsdg::Type> type)
  {
    ConstantPointerNullOperation operation(CheckAndExtractType(type));
    return tac::create(operation, {});
  }

  static jlm::rvsdg::output *
  Create(rvsdg::Region * region, std::shared_ptr<const rvsdg::Type> type)
  {
    return rvsdg::CreateOpNode<ConstantPointerNullOperation>(*region, CheckAndExtractType(type))
        .output(0);
  }

private:
  static const std::shared_ptr<const PointerType>
  CheckAndExtractType(std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    if (auto pointerType = std::dynamic_pointer_cast<const PointerType>(type))
      return pointerType;

    throw jlm::util::error("expected pointer type.");
  }
};

class IntegerToPointerOperation final : public rvsdg::UnaryOperation
{
public:
  ~IntegerToPointerOperation() noexcept override;

  IntegerToPointerOperation(
      std::shared_ptr<const jlm::rvsdg::bittype> btype,
      std::shared_ptr<const PointerType> ptype)
      : UnaryOperation(std::move(btype), std::move(ptype))
  {}

  IntegerToPointerOperation(
      std::shared_ptr<const jlm::rvsdg::Type> srctype,
      std::shared_ptr<const jlm::rvsdg::Type> dsttype)
      : UnaryOperation(srctype, dsttype)
  {
    auto at = dynamic_cast<const jlm::rvsdg::bittype *>(srctype.get());
    if (!at)
      throw jlm::util::error("expected bitstring type.");

    auto pt = dynamic_cast<const PointerType *>(dsttype.get());
    if (!pt)
      throw jlm::util::error("expected pointer type.");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  jlm::rvsdg::unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::output * output) const noexcept override;

  jlm::rvsdg::output *
  reduce_operand(jlm::rvsdg::unop_reduction_path_t path, jlm::rvsdg::output * output)
      const override;

  inline size_t
  nbits() const noexcept
  {
    return std::static_pointer_cast<const jlm::rvsdg::bittype>(argument(0))->nbits();
  }

  static std::unique_ptr<llvm::tac>
  create(const variable * argument, std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    auto at = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(argument->Type());
    if (!at)
      throw jlm::util::error("expected bitstring type.");

    auto pt = std::dynamic_pointer_cast<const PointerType>(type);
    if (!pt)
      throw jlm::util::error("expected pointer type.");

    IntegerToPointerOperation op(at, pt);
    return tac::create(op, { argument });
  }

  static jlm::rvsdg::output *
  create(jlm::rvsdg::output * operand, std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    auto ot = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(operand->Type());
    if (!ot)
      throw jlm::util::error("expected bitstring type.");

    auto pt = std::dynamic_pointer_cast<const PointerType>(type);
    if (!pt)
      throw jlm::util::error("expected pointer type.");

    return rvsdg::CreateOpNode<IntegerToPointerOperation>({ operand }, ot, pt).output(0);
  }
};

class PtrToIntOperation final : public rvsdg::UnaryOperation
{
public:
  ~PtrToIntOperation() noexcept override;

  PtrToIntOperation(
      std::shared_ptr<const PointerType> ptype,
      std::shared_ptr<const jlm::rvsdg::bittype> btype)
      : UnaryOperation(std::move(ptype), std::move(btype))
  {}

  PtrToIntOperation(
      std::shared_ptr<const jlm::rvsdg::Type> srctype,
      std::shared_ptr<const jlm::rvsdg::Type> dsttype)
      : UnaryOperation(srctype, dsttype)
  {
    auto pt = dynamic_cast<const PointerType *>(srctype.get());
    if (!pt)
      throw jlm::util::error("expected pointer type.");

    auto bt = dynamic_cast<const jlm::rvsdg::bittype *>(dsttype.get());
    if (!bt)
      throw jlm::util::error("expected bitstring type.");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  jlm::rvsdg::unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::output * output) const noexcept override;

  jlm::rvsdg::output *
  reduce_operand(jlm::rvsdg::unop_reduction_path_t path, jlm::rvsdg::output * output)
      const override;

  inline size_t
  nbits() const noexcept
  {
    return std::static_pointer_cast<const rvsdg::bittype>(result(0))->nbits();
  }

  static std::unique_ptr<llvm::tac>
  create(const variable * argument, const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto pt = std::dynamic_pointer_cast<const PointerType>(argument->Type());
    if (!pt)
      throw jlm::util::error("expected pointer type.");

    auto bt = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(type);
    if (!bt)
      throw jlm::util::error("expected bitstring type.");

    PtrToIntOperation op(std::move(pt), std::move(bt));
    return tac::create(op, { argument });
  }
};

/* Constant Data Array operator */

class ConstantDataArray final : public rvsdg::SimpleOperation
{
public:
  virtual ~ConstantDataArray();

  ConstantDataArray(const std::shared_ptr<const jlm::rvsdg::ValueType> & type, size_t size)
      : SimpleOperation({ size, type }, { ArrayType::Create(type, size) })
  {
    if (size == 0)
      throw jlm::util::error("size equals zero.");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  size_t
  size() const noexcept
  {
    return std::static_pointer_cast<const ArrayType>(result(0))->nelements();
  }

  const jlm::rvsdg::ValueType &
  type() const noexcept
  {
    return std::static_pointer_cast<const ArrayType>(result(0))->element_type();
  }

  static std::unique_ptr<llvm::tac>
  create(const std::vector<const variable *> & elements)
  {
    if (elements.size() == 0)
      throw jlm::util::error("expected at least one element.");

    auto vt = std::dynamic_pointer_cast<const jlm::rvsdg::ValueType>(elements[0]->Type());
    if (!vt)
      throw jlm::util::error("expected value type.");

    ConstantDataArray op(std::move(vt), elements.size());
    return tac::create(op, elements);
  }

  static jlm::rvsdg::output *
  Create(const std::vector<jlm::rvsdg::output *> & elements)
  {
    if (elements.empty())
      throw jlm::util::error("Expected at least one element.");

    auto valueType = std::dynamic_pointer_cast<const jlm::rvsdg::ValueType>(elements[0]->Type());
    if (!valueType)
    {
      throw jlm::util::error("Expected value type.");
    }

    return rvsdg::CreateOpNode<ConstantDataArray>(elements, std::move(valueType), elements.size())
        .output(0);
  }
};

/* pointer compare operator */

enum class cmp
{
  eq,
  ne,
  gt,
  ge,
  lt,
  le
};

class ptrcmp_op final : public rvsdg::BinaryOperation
{
public:
  virtual ~ptrcmp_op();

  inline ptrcmp_op(const std::shared_ptr<const PointerType> & ptype, const llvm::cmp & cmp)
      : BinaryOperation({ ptype, ptype }, jlm::rvsdg::bittype::Create(1)),
        cmp_(cmp)
  {}

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  virtual jlm::rvsdg::binop_reduction_path_t
  can_reduce_operand_pair(const jlm::rvsdg::output * op1, const jlm::rvsdg::output * op2)
      const noexcept override;

  virtual jlm::rvsdg::output *
  reduce_operand_pair(
      jlm::rvsdg::binop_reduction_path_t path,
      jlm::rvsdg::output * op1,
      jlm::rvsdg::output * op2) const override;

  inline llvm::cmp
  cmp() const noexcept
  {
    return cmp_;
  }

  static std::unique_ptr<llvm::tac>
  create(const llvm::cmp & cmp, const variable * op1, const variable * op2)
  {
    auto pt = std::dynamic_pointer_cast<const PointerType>(op1->Type());
    if (!pt)
      throw jlm::util::error("expected pointer type.");

    ptrcmp_op op(std::move(pt), cmp);
    return tac::create(op, { op1, op2 });
  }

private:
  llvm::cmp cmp_;
};

class ZExtOperation final : public rvsdg::UnaryOperation
{
public:
  ~ZExtOperation() noexcept override;

  ZExtOperation(size_t nsrcbits, size_t ndstbits)
      : UnaryOperation(rvsdg::bittype::Create(nsrcbits), rvsdg::bittype::Create(ndstbits))
  {
    if (ndstbits < nsrcbits)
      throw jlm::util::error("# destination bits must be greater than # source bits.");
  }

  ZExtOperation(
      const std::shared_ptr<const jlm::rvsdg::bittype> & srctype,
      const std::shared_ptr<const jlm::rvsdg::bittype> & dsttype)
      : UnaryOperation(srctype, dsttype)
  {
    if (dsttype->nbits() < srctype->nbits())
      throw jlm::util::error("# destination bits must be greater than # source bits.");
  }

  ZExtOperation(
      std::shared_ptr<const jlm::rvsdg::Type> srctype,
      std::shared_ptr<const jlm::rvsdg::Type> dsttype)
      : UnaryOperation(srctype, dsttype)
  {
    auto st = dynamic_cast<const jlm::rvsdg::bittype *>(srctype.get());
    if (!st)
      throw jlm::util::error("expected bitstring type.");

    auto dt = dynamic_cast<const jlm::rvsdg::bittype *>(dsttype.get());
    if (!dt)
      throw jlm::util::error("expected bitstring type.");

    if (dt->nbits() < st->nbits())
      throw jlm::util::error("# destination bits must be greater than # source bits.");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  virtual jlm::rvsdg::unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::output * operand) const noexcept override;

  virtual jlm::rvsdg::output *
  reduce_operand(jlm::rvsdg::unop_reduction_path_t path, jlm::rvsdg::output * operand)
      const override;

  inline size_t
  nsrcbits() const noexcept
  {
    return std::static_pointer_cast<const rvsdg::bittype>(argument(0))->nbits();
  }

  inline size_t
  ndstbits() const noexcept
  {
    return std::static_pointer_cast<const rvsdg::bittype>(result(0))->nbits();
  }

  static std::unique_ptr<llvm::tac>
  create(const variable * operand, const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto operandBitType = CheckAndExtractBitType(operand->Type());
    auto resultBitType = CheckAndExtractBitType(type);

    const ZExtOperation operation(std::move(operandBitType), std::move(resultBitType));
    return tac::create(operation, { operand });
  }

  static rvsdg::output &
  Create(rvsdg::output & operand, const std::shared_ptr<const rvsdg::Type> & resultType)
  {
    auto operandBitType = CheckAndExtractBitType(operand.Type());
    auto resultBitType = CheckAndExtractBitType(resultType);

    return *rvsdg::CreateOpNode<ZExtOperation>(
                { &operand },
                std::move(operandBitType),
                std::move(resultBitType))
                .output(0);
  }

private:
  static std::shared_ptr<const rvsdg::bittype>
  CheckAndExtractBitType(const std::shared_ptr<const rvsdg::Type> & type)
  {
    if (auto bitType = std::dynamic_pointer_cast<const rvsdg::bittype>(type))
    {
      return bitType;
    }

    throw util::type_error("bittype", type->debug_string());
  }
};

/* floating point constant operator */

class ConstantFP final : public rvsdg::SimpleOperation
{
public:
  virtual ~ConstantFP();

  inline ConstantFP(const fpsize & size, const ::llvm::APFloat & constant)
      : SimpleOperation({}, { FloatingPointType::Create(size) }),
        constant_(constant)
  {}

  ConstantFP(std::shared_ptr<const FloatingPointType> fpt, const ::llvm::APFloat & constant)
      : SimpleOperation({}, { std::move(fpt) }),
        constant_(constant)
  {}

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  inline const ::llvm::APFloat &
  constant() const noexcept
  {
    return constant_;
  }

  inline const fpsize &
  size() const noexcept
  {
    return std::static_pointer_cast<const FloatingPointType>(result(0))->size();
  }

  static std::unique_ptr<llvm::tac>
  create(const ::llvm::APFloat & constant, const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto ft = std::dynamic_pointer_cast<const FloatingPointType>(type);
    if (!ft)
      throw jlm::util::error("expected floating point type.");

    ConstantFP op(std::move(ft), constant);
    return tac::create(op, {});
  }

private:
  /* FIXME: I would not like to use the APFloat here,
     but I don't have a replacement right now. */
  ::llvm::APFloat constant_;
};

/* floating point comparison operator */

enum class fpcmp
{
  TRUE,
  FALSE,
  oeq,
  ogt,
  oge,
  olt,
  ole,
  one,
  ord,
  ueq,
  ugt,
  uge,
  ult,
  ule,
  une,
  uno
};

class fpcmp_op final : public rvsdg::BinaryOperation
{
public:
  virtual ~fpcmp_op();

  inline fpcmp_op(const fpcmp & cmp, const fpsize & size)
      : BinaryOperation(
            { FloatingPointType::Create(size), FloatingPointType::Create(size) },
            jlm::rvsdg::bittype::Create(1)),
        cmp_(cmp)
  {}

  fpcmp_op(const fpcmp & cmp, const std::shared_ptr<const FloatingPointType> & fpt)
      : BinaryOperation({ fpt, fpt }, jlm::rvsdg::bittype::Create(1)),
        cmp_(cmp)
  {}

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  jlm::rvsdg::binop_reduction_path_t
  can_reduce_operand_pair(const jlm::rvsdg::output * op1, const jlm::rvsdg::output * op2)
      const noexcept override;

  jlm::rvsdg::output *
  reduce_operand_pair(
      jlm::rvsdg::binop_reduction_path_t path,
      jlm::rvsdg::output * op1,
      jlm::rvsdg::output * op2) const override;

  inline const fpcmp &
  cmp() const noexcept
  {
    return cmp_;
  }

  inline const fpsize &
  size() const noexcept
  {
    return std::static_pointer_cast<const FloatingPointType>(argument(0))->size();
  }

  static std::unique_ptr<llvm::tac>
  create(const fpcmp & cmp, const variable * op1, const variable * op2)
  {
    auto ft = std::dynamic_pointer_cast<const FloatingPointType>(op1->Type());
    if (!ft)
      throw jlm::util::error("expected floating point type.");

    fpcmp_op op(cmp, std::move(ft));
    return tac::create(op, { op1, op2 });
  }

private:
  fpcmp cmp_;
};

/** \brief UndefValueOperation class
 *
 * This operator is the Jlm equivalent of LLVM's UndefValue constant.
 */
class UndefValueOperation final : public rvsdg::SimpleOperation
{
public:
  ~UndefValueOperation() noexcept override;

  explicit UndefValueOperation(std::shared_ptr<const jlm::rvsdg::Type> type)
      : SimpleOperation({}, { std::move(type) })
  {}

  UndefValueOperation(const UndefValueOperation &) = default;

  UndefValueOperation &
  operator=(const UndefValueOperation &) = delete;

  UndefValueOperation &
  operator=(UndefValueOperation &&) = delete;

  bool
  operator==(const Operation & other) const noexcept override;

  [[nodiscard]] std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  [[nodiscard]] const rvsdg::Type &
  GetType() const noexcept
  {
    return *result(0);
  }

  static jlm::rvsdg::output *
  Create(rvsdg::Region & region, std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    return rvsdg::CreateOpNode<UndefValueOperation>(region, std::move(type)).output(0);
  }

  static std::unique_ptr<llvm::tac>
  Create(std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    UndefValueOperation operation(std::move(type));
    return tac::create(operation, {});
  }

  static std::unique_ptr<llvm::tac>
  Create(std::shared_ptr<const jlm::rvsdg::Type> type, const std::string & name)
  {
    UndefValueOperation operation(std::move(type));
    return tac::create(operation, {}, { name });
  }

  static std::unique_ptr<llvm::tac>
  Create(std::unique_ptr<tacvariable> result)
  {
    auto & type = result->Type();

    std::vector<std::unique_ptr<tacvariable>> results;
    results.push_back(std::move(result));

    UndefValueOperation operation(type);
    return tac::create(operation, {}, std::move(results));
  }
};

/** \brief PoisonValueOperation class
 *
 * This operator is the Jlm equivalent of LLVM's PoisonValue constant.
 */
class PoisonValueOperation final : public rvsdg::SimpleOperation
{
public:
  ~PoisonValueOperation() noexcept override;

  explicit PoisonValueOperation(std::shared_ptr<const jlm::rvsdg::ValueType> type)
      : SimpleOperation({}, { std::move(type) })
  {}

  PoisonValueOperation(const PoisonValueOperation &) = default;

  PoisonValueOperation(PoisonValueOperation &&) = delete;

  PoisonValueOperation &
  operator=(const PoisonValueOperation &) = delete;

  PoisonValueOperation &
  operator=(PoisonValueOperation &&) = delete;

  bool
  operator==(const Operation & other) const noexcept override;

  std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  const jlm::rvsdg::ValueType &
  GetType() const noexcept
  {
    return *util::AssertedCast<const rvsdg::ValueType>(result(0).get());
  }

  static std::unique_ptr<llvm::tac>
  Create(const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto valueType = CheckAndConvertType(type);

    PoisonValueOperation operation(std::move(valueType));
    return tac::create(operation, {});
  }

  static jlm::rvsdg::output *
  Create(rvsdg::Region * region, const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto valueType = CheckAndConvertType(type);

    return rvsdg::CreateOpNode<PoisonValueOperation>(*region, std::move(valueType)).output(0);
  }

private:
  static std::shared_ptr<const jlm::rvsdg::ValueType>
  CheckAndConvertType(const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    if (auto valueType = std::dynamic_pointer_cast<const jlm::rvsdg::ValueType>(type))
      return valueType;

    throw jlm::util::error("Expected value type.");
  }
};

/* floating point arithmetic operator */

enum class fpop
{
  add,
  sub,
  mul,
  div,
  mod
};

class fpbin_op final : public rvsdg::BinaryOperation
{
public:
  virtual ~fpbin_op();

  inline fpbin_op(const llvm::fpop & op, const fpsize & size)
      : BinaryOperation(
            { FloatingPointType::Create(size), FloatingPointType::Create(size) },
            FloatingPointType::Create(size)),
        op_(op)
  {}

  fpbin_op(const llvm::fpop & op, const std::shared_ptr<const FloatingPointType> & fpt)
      : BinaryOperation({ fpt, fpt }, fpt),
        op_(op)
  {}

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  jlm::rvsdg::binop_reduction_path_t
  can_reduce_operand_pair(const jlm::rvsdg::output * op1, const jlm::rvsdg::output * op2)
      const noexcept override;

  jlm::rvsdg::output *
  reduce_operand_pair(
      jlm::rvsdg::binop_reduction_path_t path,
      jlm::rvsdg::output * op1,
      jlm::rvsdg::output * op2) const override;

  inline const llvm::fpop &
  fpop() const noexcept
  {
    return op_;
  }

  inline const fpsize &
  size() const noexcept
  {
    return std::static_pointer_cast<const FloatingPointType>(result(0))->size();
  }

  static std::unique_ptr<llvm::tac>
  create(const llvm::fpop & fpop, const variable * op1, const variable * op2)
  {
    auto ft = std::dynamic_pointer_cast<const FloatingPointType>(op1->Type());
    if (!ft)
      throw jlm::util::error("expected floating point type.");

    fpbin_op op(fpop, ft);
    return tac::create(op, { op1, op2 });
  }

private:
  llvm::fpop op_;
};

class FPExtOperation final : public rvsdg::UnaryOperation
{
public:
  ~FPExtOperation() noexcept override;

  FPExtOperation(const fpsize & srcsize, const fpsize & dstsize)
      : UnaryOperation(FloatingPointType::Create(srcsize), FloatingPointType::Create(dstsize))
  {
    if (srcsize == fpsize::flt && dstsize == fpsize::half)
      throw jlm::util::error("destination type size must be bigger than source type size.");
  }

  FPExtOperation(
      const std::shared_ptr<const FloatingPointType> & srctype,
      const std::shared_ptr<const FloatingPointType> & dsttype)
      : UnaryOperation(srctype, dsttype)
  {
    if (srctype->size() == fpsize::flt && dsttype->size() == fpsize::half)
      throw jlm::util::error("destination type size must be bigger than source type size.");
  }

  FPExtOperation(
      std::shared_ptr<const jlm::rvsdg::Type> srctype,
      std::shared_ptr<const jlm::rvsdg::Type> dsttype)
      : UnaryOperation(srctype, dsttype)
  {
    auto st = dynamic_cast<const FloatingPointType *>(srctype.get());
    if (!st)
      throw jlm::util::error("expected floating point type.");

    auto dt = dynamic_cast<const FloatingPointType *>(dsttype.get());
    if (!dt)
      throw jlm::util::error("expected floating point type.");

    if (st->size() == fpsize::flt && dt->size() == fpsize::half)
      throw jlm::util::error("destination type size must be bigger than source type size.");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  jlm::rvsdg::unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::output * output) const noexcept override;

  jlm::rvsdg::output *
  reduce_operand(jlm::rvsdg::unop_reduction_path_t path, jlm::rvsdg::output * output)
      const override;

  inline const fpsize &
  srcsize() const noexcept
  {
    return std::static_pointer_cast<const FloatingPointType>(argument(0))->size();
  }

  inline const fpsize &
  dstsize() const noexcept
  {
    return std::static_pointer_cast<const FloatingPointType>(result(0))->size();
  }

  static std::unique_ptr<llvm::tac>
  create(const variable * operand, const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto st = std::dynamic_pointer_cast<const FloatingPointType>(operand->Type());
    if (!st)
      throw jlm::util::error("expected floating point type.");

    auto dt = std::dynamic_pointer_cast<const FloatingPointType>(type);
    if (!dt)
      throw jlm::util::error("expected floating point type.");

    const FPExtOperation op(std::move(st), std::move(dt));
    return tac::create(op, { operand });
  }
};

class FNegOperation final : public rvsdg::UnaryOperation
{
public:
  ~FNegOperation() noexcept override;

  explicit FNegOperation(const fpsize & size)
      : UnaryOperation(FloatingPointType::Create(size), FloatingPointType::Create(size))
  {}

  explicit FNegOperation(const std::shared_ptr<const FloatingPointType> & fpt)
      : UnaryOperation(fpt, fpt)
  {}

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  jlm::rvsdg::unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::output * output) const noexcept override;

  jlm::rvsdg::output *
  reduce_operand(jlm::rvsdg::unop_reduction_path_t path, jlm::rvsdg::output * output)
      const override;

  const fpsize &
  size() const noexcept
  {
    return std::static_pointer_cast<const FloatingPointType>(argument(0))->size();
  }

  static std::unique_ptr<llvm::tac>
  create(const variable * operand)
  {
    auto type = std::dynamic_pointer_cast<const FloatingPointType>(operand->Type());
    if (!type)
      throw jlm::util::error("expected floating point type.");

    const FNegOperation op(std::move(type));
    return tac::create(op, { operand });
  }
};

class FPTruncOperation final : public rvsdg::UnaryOperation
{
public:
  ~FPTruncOperation() noexcept override;

  FPTruncOperation(const fpsize & srcsize, const fpsize & dstsize)
      : UnaryOperation(FloatingPointType::Create(srcsize), FloatingPointType::Create(dstsize))
  {
    if (srcsize == fpsize::half || (srcsize == fpsize::flt && dstsize != fpsize::half)
        || (srcsize == fpsize::dbl && dstsize == fpsize::dbl))
      throw jlm::util::error("destination tpye size must be smaller than source size type.");
  }

  FPTruncOperation(
      const std::shared_ptr<const FloatingPointType> & srctype,
      const std::shared_ptr<const FloatingPointType> & dsttype)
      : UnaryOperation(srctype, dsttype)
  {
    if (srctype->size() == fpsize::flt && dsttype->size() == fpsize::half)
      throw jlm::util::error("destination type size must be bigger than source type size.");
  }

  FPTruncOperation(
      std::shared_ptr<const jlm::rvsdg::Type> srctype,
      std::shared_ptr<const jlm::rvsdg::Type> dsttype)
      : UnaryOperation(srctype, dsttype)
  {
    auto st = dynamic_cast<const FloatingPointType *>(srctype.get());
    if (!st)
      throw jlm::util::error("expected floating point type.");

    auto dt = dynamic_cast<const FloatingPointType *>(dsttype.get());
    if (!dt)
      throw jlm::util::error("expected floating point type.");

    if (st->size() == fpsize::half || (st->size() == fpsize::flt && dt->size() != fpsize::half)
        || (st->size() == fpsize::dbl && dt->size() == fpsize::dbl))
      throw jlm::util::error("destination type size must be smaller than source size type.");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  jlm::rvsdg::unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::output * output) const noexcept override;

  jlm::rvsdg::output *
  reduce_operand(jlm::rvsdg::unop_reduction_path_t path, jlm::rvsdg::output * output)
      const override;

  inline const fpsize &
  srcsize() const noexcept
  {
    return std::static_pointer_cast<const FloatingPointType>(argument(0))->size();
  }

  inline const fpsize &
  dstsize() const noexcept
  {
    return std::static_pointer_cast<const FloatingPointType>(result(0))->size();
  }

  static std::unique_ptr<llvm::tac>
  create(const variable * operand, std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    auto st = std::dynamic_pointer_cast<const FloatingPointType>(operand->Type());
    if (!st)
      throw jlm::util::error("expected floating point type.");

    auto dt = std::dynamic_pointer_cast<const FloatingPointType>(type);
    if (!dt)
      throw jlm::util::error("expected floating point type.");

    const FPTruncOperation op(std::move(st), std::move(dt));
    return tac::create(op, { operand });
  }
};

/* valist operator */

class valist_op final : public rvsdg::SimpleOperation
{
public:
  virtual ~valist_op();

  explicit valist_op(std::vector<std::shared_ptr<const jlm::rvsdg::Type>> types)
      : SimpleOperation(std::move(types), { VariableArgumentType::Create() })
  {}

  valist_op(const valist_op &) = default;

  valist_op &
  operator=(const valist_op &) = delete;

  valist_op &
  operator=(valist_op &&) = delete;

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  static std::unique_ptr<llvm::tac>
  create(const std::vector<const variable *> & arguments)
  {
    std::vector<std::shared_ptr<const jlm::rvsdg::Type>> operands;
    for (const auto & argument : arguments)
      operands.push_back(argument->Type());

    valist_op op(std::move(operands));
    return tac::create(op, arguments);
  }

  static rvsdg::output *
  Create(rvsdg::Region & region, const std::vector<rvsdg::output *> & operands)
  {
    std::vector<std::shared_ptr<const rvsdg::Type>> operandTypes;
    operandTypes.reserve(operands.size());
    for (auto & operand : operands)
      operandTypes.emplace_back(operand->Type());

    return operands.empty()
             ? rvsdg::CreateOpNode<valist_op>(region, std::move(operandTypes)).output(0)
             : rvsdg::CreateOpNode<valist_op>(operands, std::move(operandTypes)).output(0);
  }
};

/* bitcast operator */

class bitcast_op final : public rvsdg::UnaryOperation
{
public:
  virtual ~bitcast_op();

  inline bitcast_op(
      std::shared_ptr<const jlm::rvsdg::ValueType> srctype,
      std::shared_ptr<const jlm::rvsdg::ValueType> dsttype)
      : UnaryOperation(std::move(srctype), std::move(dsttype))
  {}

  inline bitcast_op(
      std::shared_ptr<const jlm::rvsdg::Type> srctype,
      std::shared_ptr<const jlm::rvsdg::Type> dsttype)
      : UnaryOperation(srctype, dsttype)
  {
    check_types(srctype, dsttype);
  }

  bitcast_op(const bitcast_op &) = default;

  bitcast_op(Operation &&) = delete;

  bitcast_op &
  operator=(const Operation &) = delete;

  bitcast_op &
  operator=(Operation &&) = delete;

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  jlm::rvsdg::unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::output * output) const noexcept override;

  jlm::rvsdg::output *
  reduce_operand(jlm::rvsdg::unop_reduction_path_t path, jlm::rvsdg::output * output)
      const override;

  static std::unique_ptr<llvm::tac>
  create(const variable * operand, std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    auto pair = check_types(operand->Type(), type);

    bitcast_op op(pair.first, pair.second);
    return tac::create(op, { operand });
  }

  static jlm::rvsdg::output *
  create(jlm::rvsdg::output * operand, std::shared_ptr<const jlm::rvsdg::Type> rtype)
  {
    auto pair = check_types(operand->Type(), rtype);
    return rvsdg::CreateOpNode<bitcast_op>({ operand }, pair.first, pair.second).output(0);
  }

private:
  static std::pair<
      std::shared_ptr<const jlm::rvsdg::ValueType>,
      std::shared_ptr<const jlm::rvsdg::ValueType>>
  check_types(
      const std::shared_ptr<const jlm::rvsdg::Type> & otype,
      const std::shared_ptr<const jlm::rvsdg::Type> & rtype)
  {
    auto ot = std::dynamic_pointer_cast<const jlm::rvsdg::ValueType>(otype);
    if (!ot)
      throw jlm::util::error("expected value type.");

    auto rt = std::dynamic_pointer_cast<const jlm::rvsdg::ValueType>(rtype);
    if (!rt)
      throw jlm::util::error("expected value type.");

    return std::make_pair(ot, rt);
  }
};

/* ConstantStruct operator */

class ConstantStruct final : public rvsdg::SimpleOperation
{
public:
  virtual ~ConstantStruct();

  inline ConstantStruct(std::shared_ptr<const StructType> type)
      : SimpleOperation(create_srctypes(*type), { type })
  {}

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  const StructType &
  type() const noexcept
  {
    return *std::static_pointer_cast<const StructType>(result(0));
  }

  static std::unique_ptr<llvm::tac>
  create(
      const std::vector<const variable *> & elements,
      const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto structType = CheckAndExtractStructType(type);

    ConstantStruct op(std::move(structType));
    return tac::create(op, elements);
  }

  static rvsdg::output &
  Create(
      rvsdg::Region &,
      const std::vector<rvsdg::output *> & operands,
      std::shared_ptr<const rvsdg::Type> resultType)
  {
    auto structType = CheckAndExtractStructType(std::move(resultType));
    return *rvsdg::CreateOpNode<ConstantStruct>(operands, std::move(structType)).output(0);
  }

private:
  static inline std::vector<std::shared_ptr<const rvsdg::Type>>
  create_srctypes(const StructType & type)
  {
    std::vector<std::shared_ptr<const rvsdg::Type>> types;
    for (size_t n = 0; n < type.GetDeclaration().NumElements(); n++)
      types.push_back(type.GetDeclaration().GetElementType(n));

    return types;
  }

  static std::shared_ptr<const StructType>
  CheckAndExtractStructType(std::shared_ptr<const rvsdg::Type> type)
  {
    if (auto structType = std::dynamic_pointer_cast<const StructType>(type))
    {
      return structType;
    }

    throw util::type_error("StructType", type->debug_string());
  }
};

class TruncOperation final : public rvsdg::UnaryOperation
{
public:
  ~TruncOperation() noexcept override;

  TruncOperation(
      const std::shared_ptr<const jlm::rvsdg::bittype> & otype,
      const std::shared_ptr<const jlm::rvsdg::bittype> & rtype)
      : UnaryOperation(otype, rtype)
  {
    if (otype->nbits() < rtype->nbits())
      throw jlm::util::error("expected operand's #bits to be larger than results' #bits.");
  }

  TruncOperation(
      std::shared_ptr<const jlm::rvsdg::Type> optype,
      std::shared_ptr<const jlm::rvsdg::Type> restype)
      : UnaryOperation(optype, restype)
  {
    auto ot = dynamic_cast<const jlm::rvsdg::bittype *>(optype.get());
    if (!ot)
      throw jlm::util::error("expected bits type.");

    auto rt = dynamic_cast<const jlm::rvsdg::bittype *>(restype.get());
    if (!rt)
      throw jlm::util::error("expected bits type.");

    if (ot->nbits() < rt->nbits())
      throw jlm::util::error("expected operand's #bits to be larger than results' #bits.");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  virtual jlm::rvsdg::unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::output * operand) const noexcept override;

  virtual jlm::rvsdg::output *
  reduce_operand(jlm::rvsdg::unop_reduction_path_t path, jlm::rvsdg::output * operand)
      const override;

  inline size_t
  nsrcbits() const noexcept
  {
    return std::static_pointer_cast<const rvsdg::bittype>(argument(0))->nbits();
  }

  inline size_t
  ndstbits() const noexcept
  {
    return std::static_pointer_cast<const rvsdg::bittype>(result(0))->nbits();
  }

  static std::unique_ptr<llvm::tac>
  create(const variable * operand, const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto ot = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(operand->Type());
    if (!ot)
      throw jlm::util::error("expected bits type.");

    auto rt = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(type);
    if (!rt)
      throw jlm::util::error("expected bits type.");

    const TruncOperation op(std::move(ot), std::move(rt));
    return tac::create(op, { operand });
  }

  static jlm::rvsdg::output *
  create(size_t ndstbits, jlm::rvsdg::output * operand)
  {
    auto ot = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(operand->Type());
    if (!ot)
      throw jlm::util::error("expected bits type.");

    return rvsdg::CreateOpNode<TruncOperation>(
               { operand },
               std::move(ot),
               rvsdg::bittype::Create(ndstbits))
        .output(0);
  }
};

/* uitofp operator */

class uitofp_op final : public rvsdg::UnaryOperation
{
public:
  virtual ~uitofp_op();

  inline uitofp_op(
      std::shared_ptr<const jlm::rvsdg::bittype> srctype,
      std::shared_ptr<const FloatingPointType> dsttype)
      : UnaryOperation(std::move(srctype), std::move(dsttype))
  {}

  inline uitofp_op(
      std::shared_ptr<const jlm::rvsdg::Type> optype,
      std::shared_ptr<const jlm::rvsdg::Type> restype)
      : UnaryOperation(optype, restype)
  {
    auto st = dynamic_cast<const jlm::rvsdg::bittype *>(optype.get());
    if (!st)
      throw jlm::util::error("expected bits type.");

    auto rt = dynamic_cast<const FloatingPointType *>(restype.get());
    if (!rt)
      throw jlm::util::error("expected floating point type.");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  virtual jlm::rvsdg::unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::output * operand) const noexcept override;

  virtual jlm::rvsdg::output *
  reduce_operand(jlm::rvsdg::unop_reduction_path_t path, jlm::rvsdg::output * operand)
      const override;

  static std::unique_ptr<llvm::tac>
  create(const variable * operand, const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto st = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(operand->Type());
    if (!st)
      throw jlm::util::error("expected bits type.");

    auto rt = std::dynamic_pointer_cast<const FloatingPointType>(type);
    if (!rt)
      throw jlm::util::error("expected floating point type.");

    uitofp_op op(std::move(st), std::move(rt));
    return tac::create(op, { operand });
  }
};

/* sitofp operator */

class sitofp_op final : public rvsdg::UnaryOperation
{
public:
  virtual ~sitofp_op();

  inline sitofp_op(
      std::shared_ptr<const jlm::rvsdg::bittype> srctype,
      std::shared_ptr<const FloatingPointType> dsttype)
      : UnaryOperation(std::move(srctype), std::move(dsttype))
  {}

  inline sitofp_op(
      std::shared_ptr<const jlm::rvsdg::Type> srctype,
      std::shared_ptr<const jlm::rvsdg::Type> dsttype)
      : UnaryOperation(srctype, dsttype)
  {
    auto st = dynamic_cast<const jlm::rvsdg::bittype *>(srctype.get());
    if (!st)
      throw jlm::util::error("expected bits type.");

    auto rt = dynamic_cast<const FloatingPointType *>(dsttype.get());
    if (!rt)
      throw jlm::util::error("expected floating point type.");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  jlm::rvsdg::unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::output * output) const noexcept override;

  jlm::rvsdg::output *
  reduce_operand(jlm::rvsdg::unop_reduction_path_t path, jlm::rvsdg::output * output)
      const override;

  static std::unique_ptr<llvm::tac>
  create(const variable * operand, const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto st = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(operand->Type());
    if (!st)
      throw jlm::util::error("expected bits type.");

    auto rt = std::dynamic_pointer_cast<const FloatingPointType>(type);
    if (!rt)
      throw jlm::util::error("expected floating point type.");

    sitofp_op op(std::move(st), std::move(rt));
    return tac::create(op, { operand });
  }
};

/* ConstantArray */

class ConstantArray final : public rvsdg::SimpleOperation
{
public:
  virtual ~ConstantArray();

  ConstantArray(const std::shared_ptr<const jlm::rvsdg::ValueType> & type, size_t size)
      : SimpleOperation({ size, type }, { ArrayType::Create(type, size) })
  {
    if (size == 0)
      throw jlm::util::error("size equals zero.\n");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  size_t
  size() const noexcept
  {
    return std::static_pointer_cast<const ArrayType>(result(0))->nelements();
  }

  const jlm::rvsdg::ValueType &
  type() const noexcept
  {
    return std::static_pointer_cast<const ArrayType>(result(0))->element_type();
  }

  static std::unique_ptr<llvm::tac>
  create(const std::vector<const variable *> & elements)
  {
    if (elements.size() == 0)
      throw jlm::util::error("expected at least one element.\n");

    auto vt = std::dynamic_pointer_cast<const jlm::rvsdg::ValueType>(elements[0]->Type());
    if (!vt)
      throw jlm::util::error("expected value Type.\n");

    ConstantArray op(vt, elements.size());
    return tac::create(op, elements);
  }

  static rvsdg::output *
  Create(const std::vector<rvsdg::output *> & operands)
  {
    if (operands.empty())
      throw util::error("Expected at least one element.\n");

    auto valueType = std::dynamic_pointer_cast<const rvsdg::ValueType>(operands[0]->Type());
    if (!valueType)
    {
      throw util::error("Expected value type.\n");
    }

    return rvsdg::CreateOpNode<ConstantArray>(operands, valueType, operands.size()).output(0);
  }
};

/* ConstantAggregateZero operator */

class ConstantAggregateZero final : public rvsdg::SimpleOperation
{
public:
  virtual ~ConstantAggregateZero();

  ConstantAggregateZero(std::shared_ptr<const jlm::rvsdg::Type> type)
      : SimpleOperation({}, { type })
  {
    auto st = dynamic_cast<const StructType *>(type.get());
    auto at = dynamic_cast<const ArrayType *>(type.get());
    auto vt = dynamic_cast<const VectorType *>(type.get());
    if (!st && !at && !vt)
      throw jlm::util::error("expected array, struct, or vector type.\n");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  static std::unique_ptr<llvm::tac>
  create(std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    ConstantAggregateZero op(std::move(type));
    return tac::create(op, {});
  }

  static jlm::rvsdg::output *
  Create(rvsdg::Region & region, std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    return rvsdg::CreateOpNode<ConstantAggregateZero>(region, std::move(type)).output(0);
  }
};

/* extractelement operator */

class extractelement_op final : public rvsdg::SimpleOperation
{
public:
  virtual ~extractelement_op();

  inline extractelement_op(
      const std::shared_ptr<const VectorType> & vtype,
      const std::shared_ptr<const jlm::rvsdg::bittype> & btype)
      : SimpleOperation({ vtype, btype }, { vtype->Type() })
  {}

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  static inline std::unique_ptr<llvm::tac>
  create(const llvm::variable * vector, const llvm::variable * index)
  {
    auto vt = std::dynamic_pointer_cast<const VectorType>(vector->Type());
    if (!vt)
      throw jlm::util::error("expected vector type.");

    auto bt = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(index->Type());
    if (!bt)
      throw jlm::util::error("expected bit type.");

    extractelement_op op(vt, bt);
    return tac::create(op, { vector, index });
  }
};

/* shufflevector operator */

class shufflevector_op final : public rvsdg::SimpleOperation
{
public:
  ~shufflevector_op() override;

  shufflevector_op(const std::shared_ptr<const FixedVectorType> & v, const std::vector<int> & mask)
      : SimpleOperation({ v, v }, { v }),
        Mask_(mask)
  {}

  shufflevector_op(
      const std::shared_ptr<const ScalableVectorType> & v,
      const std::vector<int> & mask)
      : SimpleOperation({ v, v }, { v }),
        Mask_(mask)
  {}

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  const ::llvm::ArrayRef<int>
  Mask() const
  {
    return Mask_;
  }

  static std::unique_ptr<llvm::tac>
  create(const variable * v1, const variable * v2, const std::vector<int> & mask)
  {
    if (is<FixedVectorType>(v1->type()) && is<FixedVectorType>(v2->type()))
      return CreateShuffleVectorTac<FixedVectorType>(v1, v2, mask);

    if (is<ScalableVectorType>(v1->type()) && is<ScalableVectorType>(v2->type()))
      return CreateShuffleVectorTac<ScalableVectorType>(v1, v2, mask);

    throw jlm::util::error("Expected vector types as operands.");
  }

private:
  template<typename T>
  static std::unique_ptr<tac>
  CreateShuffleVectorTac(const variable * v1, const variable * v2, const std::vector<int> & mask)
  {
    auto vt = std::static_pointer_cast<const T>(v1->Type());
    shufflevector_op op(vt, mask);
    return tac::create(op, { v1, v2 });
  }

  std::vector<int> Mask_;
};

/* constantvector operator */

class constantvector_op final : public rvsdg::SimpleOperation
{
public:
  virtual ~constantvector_op();

  explicit inline constantvector_op(const std::shared_ptr<const VectorType> & vt)
      : SimpleOperation({ vt->size(), vt->Type() }, { vt })
  {}

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  static inline std::unique_ptr<llvm::tac>
  create(
      const std::vector<const variable *> & operands,
      const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto vt = std::dynamic_pointer_cast<const VectorType>(type);
    if (!vt)
      throw jlm::util::error("expected vector type.");

    constantvector_op op(vt);
    return tac::create(op, operands);
  }
};

/* insertelement operator */

class insertelement_op final : public rvsdg::SimpleOperation
{
public:
  virtual ~insertelement_op();

  inline insertelement_op(
      const std::shared_ptr<const VectorType> & vectype,
      const std::shared_ptr<const jlm::rvsdg::ValueType> & vtype,
      const std::shared_ptr<const jlm::rvsdg::bittype> & btype)
      : SimpleOperation({ vectype, vtype, btype }, { vectype })
  {
    if (vectype->type() != *vtype)
    {
      auto received = vtype->debug_string();
      auto expected = vectype->type().debug_string();
      throw jlm::util::error(jlm::util::strfmt("expected ", expected, ", got ", received));
    }
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  static inline std::unique_ptr<llvm::tac>
  create(const llvm::variable * vector, const llvm::variable * value, const llvm::variable * index)
  {
    auto vct = std::dynamic_pointer_cast<const VectorType>(vector->Type());
    if (!vct)
      throw jlm::util::error("expected vector type.");

    auto vt = std::dynamic_pointer_cast<const jlm::rvsdg::ValueType>(value->Type());
    if (!vt)
      throw jlm::util::error("expected value type.");

    auto bt = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(index->Type());
    if (!bt)
      throw jlm::util::error("expected bit type.");

    insertelement_op op(vct, vt, bt);
    return tac::create(op, { vector, value, index });
  }
};

/* vectorunary operator */

class vectorunary_op final : public rvsdg::SimpleOperation
{
public:
  virtual ~vectorunary_op();

  inline vectorunary_op(
      const rvsdg::UnaryOperation & op,
      const std::shared_ptr<const VectorType> & operand,
      const std::shared_ptr<const VectorType> & result)
      : SimpleOperation({ operand }, { result }),
        op_(op.copy())
  {
    if (operand->type() != *op.argument(0))
    {
      auto received = operand->type().debug_string();
      auto expected = op.argument(0)->debug_string();
      throw jlm::util::error(jlm::util::strfmt("expected ", expected, ", got ", received));
    }

    if (result->type() != *op.result(0))
    {
      auto received = result->type().debug_string();
      auto expected = op.result(0)->debug_string();
      throw jlm::util::error(jlm::util::strfmt("expected ", expected, ", got ", received));
    }
  }

  inline vectorunary_op(const vectorunary_op & other)
      : SimpleOperation(other),
        op_(other.op_->copy())
  {}

  inline vectorunary_op(vectorunary_op && other)
      : SimpleOperation(other),
        op_(std::move(other.op_))
  {}

  inline vectorunary_op &
  operator=(const vectorunary_op & other)
  {
    if (this != &other)
      op_ = other.op_->copy();

    return *this;
  }

  inline vectorunary_op &
  operator=(vectorunary_op && other)
  {
    if (this != &other)
      op_ = std::move(other.op_);

    return *this;
  }

  const rvsdg::UnaryOperation &
  operation() const noexcept
  {
    return *static_cast<const rvsdg::UnaryOperation *>(op_.get());
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  static inline std::unique_ptr<llvm::tac>
  create(
      const rvsdg::UnaryOperation & unop,
      const llvm::variable * operand,
      const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto vct1 = std::dynamic_pointer_cast<const VectorType>(operand->Type());
    auto vct2 = std::dynamic_pointer_cast<const VectorType>(type);
    if (!vct1 || !vct2)
      throw jlm::util::error("expected vector type.");

    vectorunary_op op(unop, vct1, vct2);
    return tac::create(op, { operand });
  }

private:
  std::unique_ptr<Operation> op_;
};

/* vectorbinary operator */

class vectorbinary_op final : public rvsdg::SimpleOperation
{
public:
  virtual ~vectorbinary_op();

  inline vectorbinary_op(
      const rvsdg::BinaryOperation & binop,
      const std::shared_ptr<const VectorType> & op1,
      const std::shared_ptr<const VectorType> & op2,
      const std::shared_ptr<const VectorType> & result)
      : SimpleOperation({ op1, op2 }, { result }),
        op_(binop.copy())
  {
    if (*op1 != *op2)
      throw jlm::util::error("expected the same vector types.");

    if (op1->type() != *binop.argument(0))
    {
      auto received = op1->type().debug_string();
      auto expected = binop.argument(0)->debug_string();
      throw jlm::util::error(jlm::util::strfmt("expected ", expected, ", got ", received));
    }

    if (result->type() != *binop.result(0))
    {
      auto received = result->type().debug_string();
      auto expected = binop.result(0)->debug_string();
      throw jlm::util::error(jlm::util::strfmt("expected ", expected, ", got ", received));
    }
  }

  inline vectorbinary_op(const vectorbinary_op & other)
      : SimpleOperation(other),
        op_(other.op_->copy())
  {}

  inline vectorbinary_op(vectorbinary_op && other)
      : SimpleOperation(other),
        op_(std::move(other.op_))
  {}

  inline vectorbinary_op &
  operator=(const vectorbinary_op & other)
  {
    if (this != &other)
      op_ = other.op_->copy();

    return *this;
  }

  inline vectorbinary_op &
  operator=(vectorbinary_op && other)
  {
    if (this != &other)
      op_ = std::move(other.op_);

    return *this;
  }

  const rvsdg::BinaryOperation &
  operation() const noexcept
  {
    return *static_cast<const rvsdg::BinaryOperation *>(op_.get());
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  static inline std::unique_ptr<llvm::tac>
  create(
      const rvsdg::BinaryOperation & binop,
      const llvm::variable * op1,
      const llvm::variable * op2,
      const std::shared_ptr<const jlm::rvsdg::Type> & type)
  {
    auto vct1 = std::dynamic_pointer_cast<const VectorType>(op1->Type());
    auto vct2 = std::dynamic_pointer_cast<const VectorType>(op2->Type());
    auto vct3 = std::dynamic_pointer_cast<const VectorType>(type);
    if (!vct1 || !vct2 || !vct3)
      throw jlm::util::error("expected vector type.");

    vectorbinary_op op(binop, vct1, vct2, vct3);
    return tac::create(op, { op1, op2 });
  }

private:
  std::unique_ptr<Operation> op_;
};

/* constant data vector operator */

class constant_data_vector_op final : public rvsdg::SimpleOperation
{
public:
  ~constant_data_vector_op() override;

private:
  explicit constant_data_vector_op(const std::shared_ptr<const VectorType> & vt)
      : SimpleOperation({ vt->size(), vt->Type() }, { vt })
  {}

public:
  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  size_t
  size() const noexcept
  {
    return std::static_pointer_cast<const VectorType>(result(0))->size();
  }

  const jlm::rvsdg::ValueType &
  type() const noexcept
  {
    return std::static_pointer_cast<const VectorType>(result(0))->type();
  }

  static std::unique_ptr<tac>
  Create(const std::vector<const variable *> & elements)
  {
    if (elements.empty())
      throw jlm::util::error("Expected at least one element.");

    auto vt = std::dynamic_pointer_cast<const jlm::rvsdg::ValueType>(elements[0]->Type());
    if (!vt)
      throw jlm::util::error("Expected value type.");

    constant_data_vector_op op(FixedVectorType::Create(vt, elements.size()));
    return tac::create(op, elements);
  }
};

/* ExtractValue operator */

class ExtractValue final : public rvsdg::SimpleOperation
{
  typedef std::vector<unsigned>::const_iterator const_iterator;

public:
  virtual ~ExtractValue();

  inline ExtractValue(
      const std::shared_ptr<const jlm::rvsdg::Type> & aggtype,
      const std::vector<unsigned> & indices)
      : SimpleOperation({ aggtype }, { dsttype(aggtype, indices) }),
        indices_(indices)
  {
    if (indices.empty())
      throw jlm::util::error("expected at least one index.");
  }

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  const_iterator
  begin() const
  {
    return indices_.begin();
  }

  const_iterator
  end() const
  {
    return indices_.end();
  }

  const jlm::rvsdg::ValueType &
  type() const noexcept
  {
    return *std::static_pointer_cast<const rvsdg::ValueType>(argument(0));
  }

  static inline std::unique_ptr<llvm::tac>
  create(const llvm::variable * aggregate, const std::vector<unsigned> & indices)
  {
    ExtractValue op(aggregate->Type(), indices);
    return tac::create(op, { aggregate });
  }

private:
  static inline std::vector<std::shared_ptr<const rvsdg::Type>>
  dsttype(
      const std::shared_ptr<const jlm::rvsdg::Type> & aggtype,
      const std::vector<unsigned> & indices)
  {
    std::shared_ptr<const jlm::rvsdg::Type> type = aggtype;
    for (const auto & index : indices)
    {
      if (auto st = std::dynamic_pointer_cast<const StructType>(type))
      {
        if (index >= st->GetDeclaration().NumElements())
          throw jlm::util::error("extractvalue index out of bound.");

        type = st->GetDeclaration().GetElementType(index);
      }
      else if (auto at = std::dynamic_pointer_cast<const ArrayType>(type))
      {
        if (index >= at->nelements())
          throw jlm::util::error("extractvalue index out of bound.");

        type = at->GetElementType();
      }
      else
        throw jlm::util::error("expected struct or array type.");
    }

    return { type };
  }

  std::vector<unsigned> indices_;
};

/* malloc operator */

class malloc_op final : public rvsdg::SimpleOperation
{
public:
  virtual ~malloc_op();

  explicit malloc_op(std::shared_ptr<const jlm::rvsdg::bittype> btype)
      : SimpleOperation({ std::move(btype) }, { PointerType::Create(), MemoryStateType::Create() })
  {}

  virtual bool
  operator==(const Operation & other) const noexcept override;

  virtual std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  const jlm::rvsdg::bittype &
  size_type() const noexcept
  {
    return *std::static_pointer_cast<const rvsdg::bittype>(argument(0));
  }

  rvsdg::FunctionType
  fcttype() const
  {
    JLM_ASSERT(narguments() == 1 && nresults() == 2);
    return rvsdg::FunctionType({ argument(0) }, { result(0), result(1) });
  }

  static std::unique_ptr<llvm::tac>
  create(const variable * size)
  {
    auto bt = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(size->Type());
    if (!bt)
      throw jlm::util::error("expected bits type.");

    malloc_op op(std::move(bt));
    return tac::create(op, { size });
  }

  static std::vector<jlm::rvsdg::output *>
  create(jlm::rvsdg::output * size)
  {
    auto bt = std::dynamic_pointer_cast<const jlm::rvsdg::bittype>(size->Type());
    if (!bt)
      throw jlm::util::error("expected bits type.");

    return outputs(&rvsdg::CreateOpNode<malloc_op>({ size }, std::move(bt)));
  }
};

/**
 * Represents the standard C library call free() used for freeing dynamically allocated memory.
 *
 * This operation has no equivalent LLVM instruction.
 */
class FreeOperation final : public rvsdg::SimpleOperation
{
public:
  ~FreeOperation() noexcept override;

  explicit FreeOperation(size_t numMemoryStates)
      : SimpleOperation(CreateOperandTypes(numMemoryStates), CreateResultTypes(numMemoryStates))
  {}

  bool
  operator==(const Operation & other) const noexcept override;

  [[nodiscard]] std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  static std::unique_ptr<llvm::tac>
  Create(
      const variable * pointer,
      const std::vector<const variable *> & memoryStates,
      const variable * iOState)
  {
    std::vector<const variable *> operands;
    operands.push_back(pointer);
    operands.insert(operands.end(), memoryStates.begin(), memoryStates.end());
    operands.push_back(iOState);

    FreeOperation operation(memoryStates.size());
    return tac::create(operation, operands);
  }

  static std::vector<jlm::rvsdg::output *>
  Create(
      jlm::rvsdg::output * pointer,
      const std::vector<jlm::rvsdg::output *> & memoryStates,
      jlm::rvsdg::output * iOState)
  {
    std::vector<jlm::rvsdg::output *> operands;
    operands.push_back(pointer);
    operands.insert(operands.end(), memoryStates.begin(), memoryStates.end());
    operands.push_back(iOState);

    return outputs(&rvsdg::CreateOpNode<FreeOperation>(operands, memoryStates.size()));
  }

private:
  static std::vector<std::shared_ptr<const rvsdg::Type>>
  CreateOperandTypes(size_t numMemoryStates)
  {
    std::vector<std::shared_ptr<const rvsdg::Type>> memoryStates(
        numMemoryStates,
        MemoryStateType::Create());

    std::vector<std::shared_ptr<const rvsdg::Type>> types({ PointerType::Create() });
    types.insert(types.end(), memoryStates.begin(), memoryStates.end());
    types.emplace_back(IOStateType::Create());

    return types;
  }

  static std::vector<std::shared_ptr<const rvsdg::Type>>
  CreateResultTypes(size_t numMemoryStates)
  {
    std::vector<std::shared_ptr<const rvsdg::Type>> types(
        numMemoryStates,
        MemoryStateType::Create());
    types.emplace_back(IOStateType::Create());

    return types;
  }
};

}

#endif
