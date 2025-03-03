/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/llvm/ir/basic-block.hpp>
#include <jlm/llvm/ir/cfg-structure.hpp>
#include <jlm/llvm/ir/ipgraph-module.hpp>
#include <jlm/llvm/ir/operators/operators.hpp>
#include <jlm/llvm/ir/ssa.hpp>

#include <unordered_set>

namespace jlm::llvm
{

void
destruct_ssa(llvm::cfg & cfg)
{
  JLM_ASSERT(is_valid(cfg));

  auto collect_phi_blocks = [](llvm::cfg & cfg)
  {
    std::unordered_set<basic_block *> phi_blocks;
    for (auto & bb : cfg)
    {
      if (is<SsaPhiOperation>(bb.first()))
        phi_blocks.insert(&bb);
    }

    return phi_blocks;
  };

  auto eliminate_phis = [](llvm::cfg & cfg, const std::unordered_set<basic_block *> & phi_blocks)
  {
    if (phi_blocks.empty())
      return;

    auto firstbb = static_cast<basic_block *>(cfg.entry()->OutEdge(0)->sink());

    for (auto phi_block : phi_blocks)
    {
      auto ass_block = basic_block::create(cfg);
      auto & tacs = phi_block->tacs();

      /* collect inedges of phi block */
      std::unordered_map<cfg_node *, cfg_edge *> edges;
      for (auto & inedge : phi_block->InEdges())
      {
        JLM_ASSERT(edges.find(inedge.source()) == edges.end());
        edges[inedge.source()] = &inedge;
      }

      while (tacs.first())
      {
        auto phitac = tacs.first();
        if (!is<SsaPhiOperation>(phitac))
          break;

        const auto phi = static_cast<const SsaPhiOperation *>(&phitac->operation());
        auto v = cfg.module().create_variable(phi->Type());

        const variable * value = nullptr;
        for (size_t n = 0; n < phitac->noperands(); n++)
        {
          JLM_ASSERT(edges.find(phi->node(n)) != edges.end());
          auto bb = edges[phi->node(n)]->split();
          value = bb->append_last(AssignmentOperation::create(phitac->operand(n), v))->operand(0);
        }

        auto phiresult = std::move(phitac->results()[0]);
        auto undef = firstbb->append_first(UndefValueOperation::Create(std::move(phiresult)));
        ass_block->append_last(AssignmentOperation::create(value, undef->result(0)));
        tacs.drop_first();
      }

      phi_block->divert_inedges(ass_block);
      ass_block->add_outedge(phi_block);
    }
  };

  auto phi_blocks = collect_phi_blocks(cfg);
  eliminate_phis(cfg, phi_blocks);
}

}
