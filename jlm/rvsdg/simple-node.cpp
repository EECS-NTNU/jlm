/*
 * Copyright 2016 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/rvsdg/graph.hpp>
#include <jlm/rvsdg/notifiers.hpp>
#include <jlm/rvsdg/simple-node.hpp>
#include <jlm/rvsdg/substitution.hpp>

namespace jlm::rvsdg
{

SimpleNode::~SimpleNode()
{
  on_node_destroy(this);
}

SimpleNode::SimpleNode(
    rvsdg::Region & region,
    std::unique_ptr<SimpleOperation> operation,
    const std::vector<jlm::rvsdg::Output *> & operands)
    : Node(&region),
      Operation_(std::move(operation))
{
  if (GetOperation().narguments() != operands.size())
    throw util::Error(jlm::util::strfmt(
        "Argument error - expected ",
        SimpleNode::GetOperation().narguments(),
        ", received ",
        operands.size(),
        " arguments."));

  for (size_t n = 0; n < SimpleNode::GetOperation().narguments(); n++)
  {
    add_input(
        std::make_unique<node_input>(operands[n], this, SimpleNode::GetOperation().argument(n)));
  }

  for (size_t n = 0; n < SimpleNode::GetOperation().nresults(); n++)
    add_output(std::make_unique<node_output>(this, SimpleNode::GetOperation().result(n)));

  on_node_create(this);
}

const SimpleOperation &
SimpleNode::GetOperation() const noexcept
{
  return *Operation_;
}

Node *
SimpleNode::copy(rvsdg::Region * region, const std::vector<jlm::rvsdg::Output *> & operands) const
{
  std::unique_ptr<SimpleOperation> operation(
      util::AssertedCast<SimpleOperation>(GetOperation().copy().release()));
  return &Create(*region, std::move(operation), operands);
}

Node *
SimpleNode::copy(rvsdg::Region * region, SubstitutionMap & smap) const
{
  std::vector<jlm::rvsdg::Output *> operands;
  for (size_t n = 0; n < ninputs(); n++)
  {
    auto origin = input(n)->origin();
    auto operand = smap.lookup(origin);

    if (operand == nullptr)
    {
      if (region != this->region())
        throw util::Error("Node operand not in substitution map.");

      operand = origin;
    }

    operands.push_back(operand);
  }

  auto node = copy(region, operands);

  JLM_ASSERT(node->noutputs() == noutputs());
  for (size_t n = 0; n < node->noutputs(); n++)
    smap.insert(output(n), node->output(n));

  return node;
}

std::string
SimpleNode::DebugString() const
{
  return GetOperation().debug_string();
}

std::optional<std::vector<rvsdg::Output *>>
NormalizeSimpleOperationCommonNodeElimination(
    Region & region,
    const SimpleOperation & operation,
    const std::vector<rvsdg::Output *> & operands)
{
  auto isCongruent = [&](const Node & node)
  {
    auto & nodeOperation = node.GetOperation();
    return nodeOperation == operation && operands == rvsdg::operands(&node)
        && &nodeOperation != &operation;
  };

  if (operands.empty())
  {
    for (auto & node : region.TopNodes())
    {
      if (isCongruent(node))
      {
        return outputs(&node);
      }
    }
  }
  else
  {
    for (const auto & user : operands[0]->Users())
    {
      if (const auto node = TryGetOwnerNode<SimpleNode>(user))
      {
        if (isCongruent(*node))
        {
          return outputs(node);
        }
      }
    }
  }

  return std::nullopt;
}

}
