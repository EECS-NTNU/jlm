/*
 * Copyright 2021 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#include <jlm/hls/backend/rvsdg2rhls/distribute-constants.hpp>
#include <jlm/hls/backend/rvsdg2rhls/rvsdg2rhls.hpp>
#include <jlm/hls/ir/hls.hpp>
#include <jlm/hls/util/view.hpp>
#include <jlm/rvsdg/gamma.hpp>
#include <jlm/rvsdg/theta.hpp>
#include <jlm/rvsdg/traverser.hpp>

namespace jlm
{

void
distribute_constant(const rvsdg::SimpleOperation & op, rvsdg::simple_output * out)
{
  JLM_ASSERT(jlm::hls::is_constant(out->node()));
  bool changed = true;
  while (changed)
  {
    changed = false;
    for (auto user : *out)
    {
      auto node = rvsdg::input::GetNode(*user);
      if (auto ti = dynamic_cast<rvsdg::ThetaInput *>(user))
      {
        auto arg = ti->argument();
        auto res = ti->result();
        if (res->origin() == arg)
        {
          // pass-through
          auto arg_replacement = dynamic_cast<rvsdg::simple_output *>(
              rvsdg::SimpleNode::create_normalized(ti->node()->subregion(), op, {})[0]);
          ti->argument()->divert_users(arg_replacement);
          ti->output()->divert_users(
              rvsdg::SimpleNode::create_normalized(out->region(), op, {})[0]);
          distribute_constant(op, arg_replacement);
          arg->region()->RemoveResult(res->index());
          arg->region()->RemoveArgument(arg->index());
          arg->region()->node()->RemoveInput(arg->input()->index());
          arg->region()->node()->RemoveOutput(res->output()->index());
          changed = true;
          break;
        }
      }
      if (auto gammaNode = dynamic_cast<rvsdg::GammaNode *>(node))
      {
        if (gammaNode->predicate() == user)
        {
          continue;
        }
        for (auto argument : gammaNode->MapInputEntryVar(*user).branchArgument)
        {
          if (argument->nusers())
          {
            auto arg_replacement = dynamic_cast<rvsdg::simple_output *>(
                rvsdg::SimpleNode::create_normalized(argument->region(), op, {})[0]);
            argument->divert_users(arg_replacement);
            distribute_constant(op, arg_replacement);
          }
          argument->region()->RemoveArgument(argument->index());
        }
        gammaNode->RemoveInput(user->index());
        changed = true;
        break;
      }
    }
  }
}

void
hls::distribute_constants(rvsdg::Region * region)
{
  // push constants down as far as possible, since this is cheaper than having forks and potentially
  // buffers for them
  for (auto & node : rvsdg::topdown_traverser(region))
  {
    if (rvsdg::is<rvsdg::StructuralOperation>(node))
    {
      if (auto ln = dynamic_cast<llvm::lambda::node *>(node))
      {
        distribute_constants(ln->subregion());
      }
      else if (auto t = dynamic_cast<rvsdg::ThetaNode *>(node))
      {
        distribute_constants(t->subregion());
      }
      else if (auto gn = dynamic_cast<rvsdg::GammaNode *>(node))
      {
        for (size_t i = 0; i < gn->nsubregions(); ++i)
        {
          distribute_constants(gn->subregion(i));
        }
      }
      else
      {
        throw util::error("Unexpected node type: " + node->GetOperation().debug_string());
      }
    }
    else if (auto sn = dynamic_cast<rvsdg::SimpleNode *>(node))
    {
      if (is_constant(node))
      {
        distribute_constant(sn->GetOperation(), sn->output(0));
      }
    }
    else
    {
      throw util::error("Unexpected node type: " + node->GetOperation().debug_string());
    }
  }
}

void
hls::distribute_constants(llvm::RvsdgModule & rm)
{
  auto & graph = rm.Rvsdg();
  auto root = &graph.GetRootRegion();
  distribute_constants(root);
}

} // namespace jlm
