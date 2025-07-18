/*
 * Copyright 2010 2011 2012 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 2013 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_RVSDG_BINARY_HPP
#define JLM_RVSDG_BINARY_HPP

#include <jlm/rvsdg/graph.hpp>
#include <jlm/rvsdg/operation.hpp>
#include <jlm/util/common.hpp>

#include <optional>

namespace jlm::rvsdg
{

typedef size_t binop_reduction_path_t;

/**
 * Binary operation taking two arguments (with well-defined reduction for more
 * operands if operator is associative).
 */
class BinaryOperation : public SimpleOperation
{
public:
  enum class flags
  {
    none = 0,
    associative = 1,
    commutative = 2
  };

  ~BinaryOperation() noexcept override;

  BinaryOperation(
      const std::vector<std::shared_ptr<const jlm::rvsdg::Type>> operands,
      std::shared_ptr<const jlm::rvsdg::Type> result)
      : SimpleOperation(std::move(operands), { std::move(result) })
  {}

  virtual binop_reduction_path_t
  can_reduce_operand_pair(const jlm::rvsdg::Output * op1, const jlm::rvsdg::Output * op2)
      const noexcept = 0;

  virtual jlm::rvsdg::Output *
  reduce_operand_pair(
      binop_reduction_path_t path,
      jlm::rvsdg::Output * op1,
      jlm::rvsdg::Output * op2) const = 0;

  virtual BinaryOperation::flags
  flags() const noexcept;

  inline bool
  is_associative() const noexcept;

  inline bool
  is_commutative() const noexcept;
};

/**
 * \brief Flattens a cascade of the same binary operations into a single flattened binary operation.
 *
 * o1 = binaryNode i1 i2
 * o2 = binaryNode o1 i3
 * =>
 * o2 = flattenedBinaryNode i1 i2 i3
 *
 * \pre The binary operation must be associative.
 *
 * @param operation The binary operation on which the transformation is performed.
 * @param operands The operands of the binary node.
 * @return If the normalization could be applied, then the results of the binary operation after
 * the transformation. Otherwise, std::nullopt.
 */
std::optional<std::vector<rvsdg::Output *>>
FlattenAssociativeBinaryOperation(
    const BinaryOperation & operation,
    const std::vector<rvsdg::Output *> & operands);

/**
 * \brief Applies the reductions implemented in the binary operations reduction functions.
 *
 * @param operation The binary operation on which the transformation is performed.
 * @param operands The operands of the binary node.
 *
 * @return If the normalization could be applied, then the results of the binary operation after
 * the transformation. Otherwise, std::nullopt.
 *
 * \see binary_op::can_reduce_operand_pair()
 * \see binary_op::reduce_operand_pair()
 */
std::optional<std::vector<rvsdg::Output *>>
NormalizeBinaryOperation(
    const BinaryOperation & operation,
    const std::vector<rvsdg::Output *> & operands);

class FlattenedBinaryOperation final : public SimpleOperation
{
public:
  enum class reduction
  {
    linear,
    parallel
  };

  ~FlattenedBinaryOperation() noexcept override;

  FlattenedBinaryOperation(std::unique_ptr<BinaryOperation> op, size_t narguments) noexcept
      : SimpleOperation({ narguments, op->argument(0) }, { op->result(0) }),
        op_(std::move(op))
  {
    JLM_ASSERT(op_->is_associative());
  }

  FlattenedBinaryOperation(const BinaryOperation & op, size_t narguments)
      : SimpleOperation({ narguments, op.argument(0) }, { op.result(0) }),
        op_(std::unique_ptr<BinaryOperation>(static_cast<BinaryOperation *>(op.copy().release())))
  {
    JLM_ASSERT(op_->is_associative());
  }

  bool
  operator==(const Operation & other) const noexcept override;

  [[nodiscard]] std::string
  debug_string() const override;

  [[nodiscard]] std::unique_ptr<Operation>
  copy() const override;

  const BinaryOperation &
  bin_operation() const noexcept
  {
    return *op_;
  }

  jlm::rvsdg::Output *
  reduce(
      const FlattenedBinaryOperation::reduction & reduction,
      const std::vector<jlm::rvsdg::Output *> & operands) const;

  static void
  reduce(rvsdg::Region * region, const FlattenedBinaryOperation::reduction & reduction);

  static inline void
  reduce(Graph * graph, const FlattenedBinaryOperation::reduction & reduction)
  {
    reduce(&graph->GetRootRegion(), reduction);
  }

private:
  std::unique_ptr<BinaryOperation> op_;
};

/**
 * \brief Applies the reductions of the binary operation represented by the flattened binary
 * operation.
 *
 * @param operation The flattened binary operation on which the transformation is performed.
 * @param operands The operands of the flattened binary node.
 *
 * @return If the normalization could be applied, then the results of the flattened binary operation
 * after the transformation. Otherwise, std::nullopt.
 *
 * \see NormalizeBinaryOperation()
 */
std::optional<std::vector<rvsdg::Output *>>
NormalizeFlattenedBinaryOperation(
    const FlattenedBinaryOperation & operation,
    const std::vector<rvsdg::Output *> & operands);

/* binary flags operators */

static constexpr enum BinaryOperation::flags
operator|(enum BinaryOperation::flags a, enum BinaryOperation::flags b)
{
  return static_cast<enum BinaryOperation::flags>(static_cast<int>(a) | static_cast<int>(b));
}

static constexpr enum BinaryOperation::flags
operator&(enum BinaryOperation::flags a, enum BinaryOperation::flags b)
{
  return static_cast<enum BinaryOperation::flags>(static_cast<int>(a) & static_cast<int>(b));
}

/* binary methods */

inline bool
BinaryOperation::is_associative() const noexcept
{
  return static_cast<int>(flags() & BinaryOperation::flags::associative);
}

inline bool
BinaryOperation::is_commutative() const noexcept
{
  return static_cast<int>(flags() & BinaryOperation::flags::commutative);
}

static const binop_reduction_path_t binop_reduction_none = 0;
/* both operands are constants */
static const binop_reduction_path_t binop_reduction_constants = 1;
/* can merge both operands into single (using some "simpler" operator) */
static const binop_reduction_path_t binop_reduction_merge = 2;
/* part of left operand can be folded into right */
static const binop_reduction_path_t binop_reduction_lfold = 3;
/* part of right operand can be folded into left */
static const binop_reduction_path_t binop_reduction_rfold = 4;
/* left operand is neutral element */
static const binop_reduction_path_t binop_reduction_lneutral = 5;
/* right operand is neutral element */
static const binop_reduction_path_t binop_reduction_rneutral = 6;
/* both operands have common form which can be factored over op */
static const binop_reduction_path_t binop_reduction_factor = 7;

}

#endif
