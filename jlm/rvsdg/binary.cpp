/*
 * Copyright 2010 2011 2012 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 2013 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/rvsdg/binary.hpp>
#include <jlm/rvsdg/graph.hpp>
#include <jlm/rvsdg/reduction-helpers.hpp>
#include <jlm/rvsdg/region.hpp>
#include <jlm/rvsdg/simple-node.hpp>
#include <jlm/rvsdg/structural-node.hpp>
#include <jlm/rvsdg/traverser.hpp>

#include <deque>

namespace jlm::rvsdg
{

/* binary normal form */

namespace
{

std::vector<jlm::rvsdg::Output *>
reduce_operands(const BinaryOperation & op, std::vector<jlm::rvsdg::Output *> args)
{
  /* pair-wise reduce */
  if (op.is_commutative())
  {
    return base::detail::commutative_pairwise_reduce(
        std::move(args),
        [&op](jlm::rvsdg::Output * arg1, jlm::rvsdg::Output * arg2)
        {
          binop_reduction_path_t reduction = op.can_reduce_operand_pair(arg1, arg2);
          return reduction != binop_reduction_none ? op.reduce_operand_pair(reduction, arg1, arg2)
                                                   : nullptr;
        });
  }
  else
  {
    return base::detail::pairwise_reduce(
        std::move(args),
        [&op](jlm::rvsdg::Output * arg1, jlm::rvsdg::Output * arg2)
        {
          binop_reduction_path_t reduction = op.can_reduce_operand_pair(arg1, arg2);
          return reduction != binop_reduction_none ? op.reduce_operand_pair(reduction, arg1, arg2)
                                                   : nullptr;
        });
  }
}

}

BinaryOperation::~BinaryOperation() noexcept = default;

enum BinaryOperation::flags
BinaryOperation::flags() const noexcept
{
  return flags::none;
}

std::optional<std::vector<rvsdg::Output *>>
FlattenAssociativeBinaryOperation(
    const BinaryOperation & operation,
    const std::vector<rvsdg::Output *> & operands)
{
  JLM_ASSERT(!operands.empty());
  auto region = operands[0]->region();

  if (!operation.is_associative())
  {
    return std::nullopt;
  }

  auto newOperands = base::detail::associative_flatten(
      operands,
      [&operation](rvsdg::Output * operand)
      {
        auto node = TryGetOwnerNode<Node>(*operand);
        if (node == nullptr)
          return false;

        auto flattenedBinaryOperation =
            dynamic_cast<const FlattenedBinaryOperation *>(&node->GetOperation());
        return node->GetOperation() == operation
            || (flattenedBinaryOperation && flattenedBinaryOperation->bin_operation() == operation);
      });

  if (operands == newOperands)
  {
    JLM_ASSERT(newOperands.size() == 2);
    return std::nullopt;
  }

  JLM_ASSERT(newOperands.size() > 2);
  auto flattenedBinaryOperation =
      std::make_unique<FlattenedBinaryOperation>(operation, newOperands.size());
  return outputs(&SimpleNode::Create(*region, std::move(flattenedBinaryOperation), newOperands));
}

std::optional<std::vector<rvsdg::Output *>>
NormalizeBinaryOperation(
    const BinaryOperation & operation,
    const std::vector<rvsdg::Output *> & operands)
{
  JLM_ASSERT(!operands.empty());
  auto region = operands[0]->region();

  auto newOperands = reduce_operands(operation, operands);

  if (newOperands.size() == 1)
  {
    // The operands could be reduced to a single value by applying constant folding.
    return newOperands;
  }

  if (newOperands == operands)
  {
    // The operands did not change, which means that none of the normalizations triggered.
    return std::nullopt;
  }

  JLM_ASSERT(newOperands.size() == 2);
  return outputs(&SimpleNode::Create(*region, operation.copy(), newOperands));
}

FlattenedBinaryOperation::~FlattenedBinaryOperation() noexcept = default;

bool
FlattenedBinaryOperation::operator==(const Operation & other) const noexcept
{
  const auto op = dynamic_cast<const FlattenedBinaryOperation *>(&other);
  return op && op->bin_operation() == bin_operation() && op->narguments() == narguments();
}

std::string
FlattenedBinaryOperation::debug_string() const
{
  return jlm::util::strfmt("FLATTENED[", op_->debug_string(), "]");
}

std::unique_ptr<Operation>
FlattenedBinaryOperation::copy() const
{
  std::unique_ptr<BinaryOperation> copied_op(static_cast<BinaryOperation *>(op_->copy().release()));
  return std::make_unique<FlattenedBinaryOperation>(std::move(copied_op), narguments());
}

/*
  FIXME: The reduce_parallel and reduce_linear functions only differ in where they add
  the new output to the working list. Unify both functions.
*/

static jlm::rvsdg::Output *
reduce_parallel(const BinaryOperation & op, const std::vector<jlm::rvsdg::Output *> & operands)
{
  JLM_ASSERT(operands.size() > 1);
  auto region = operands.front()->region();

  std::deque<jlm::rvsdg::Output *> worklist(operands.begin(), operands.end());
  while (worklist.size() > 1)
  {
    auto op1 = worklist.front();
    worklist.pop_front();
    auto op2 = worklist.front();
    worklist.pop_front();

    auto output = SimpleNode::Create(*region, op.copy(), { op1, op2 }).output(0);
    worklist.push_back(output);
  }

  JLM_ASSERT(worklist.size() == 1);
  return worklist.front();
}

static jlm::rvsdg::Output *
reduce_linear(const BinaryOperation & op, const std::vector<jlm::rvsdg::Output *> & operands)
{
  JLM_ASSERT(operands.size() > 1);
  auto region = operands.front()->region();

  std::deque<jlm::rvsdg::Output *> worklist(operands.begin(), operands.end());
  while (worklist.size() > 1)
  {
    auto op1 = worklist.front();
    worklist.pop_front();
    auto op2 = worklist.front();
    worklist.pop_front();

    auto output = SimpleNode::Create(*region, op.copy(), { op1, op2 }).output(0);
    worklist.push_front(output);
  }

  JLM_ASSERT(worklist.size() == 1);
  return worklist.front();
}

jlm::rvsdg::Output *
FlattenedBinaryOperation::reduce(
    const FlattenedBinaryOperation::reduction & reduction,
    const std::vector<jlm::rvsdg::Output *> & operands) const
{
  JLM_ASSERT(operands.size() > 1);

  static std::unordered_map<
      FlattenedBinaryOperation::reduction,
      std::function<
          jlm::rvsdg::Output *(const BinaryOperation &, const std::vector<jlm::rvsdg::Output *> &)>>
      map({ { reduction::linear, reduce_linear }, { reduction::parallel, reduce_parallel } });

  JLM_ASSERT(map.find(reduction) != map.end());
  return map[reduction](bin_operation(), operands);
}

void
FlattenedBinaryOperation::reduce(
    rvsdg::Region * region,
    const FlattenedBinaryOperation::reduction & reduction)
{
  for (auto & node : TopDownTraverser(region))
  {
    if (is<FlattenedBinaryOperation>(node))
    {
      const auto op = static_cast<const FlattenedBinaryOperation *>(&node->GetOperation());
      auto output = op->reduce(reduction, operands(node));
      node->output(0)->divert_users(output);
      remove(node);
    }
    else if (auto structnode = dynamic_cast<const StructuralNode *>(node))
    {
      for (size_t n = 0; n < structnode->nsubregions(); n++)
        reduce(structnode->subregion(n), reduction);
    }
  }

  JLM_ASSERT(!Region::ContainsOperation<FlattenedBinaryOperation>(*region, true));
}

std::optional<std::vector<rvsdg::Output *>>
NormalizeFlattenedBinaryOperation(
    const FlattenedBinaryOperation & operation,
    const std::vector<rvsdg::Output *> & operands)
{
  return NormalizeBinaryOperation(operation.bin_operation(), operands);
}

}
