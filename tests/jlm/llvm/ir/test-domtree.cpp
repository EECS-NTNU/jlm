/*
 * Copyright 2018 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-registry.hpp"

#include <jlm/llvm/ir/cfg.hpp>
#include <jlm/llvm/ir/domtree.hpp>
#include <jlm/llvm/ir/ipgraph-module.hpp>

#include <unordered_set>

template<size_t N>
static void
check(
    const jlm::llvm::DominatorTreeNode * dnode,
    const jlm::llvm::ControlFlowGraphNode * node,
    const std::unordered_set<const jlm::llvm::ControlFlowGraphNode *> & children)
{
  assert(dnode->node() == node);
  assert(dnode->nchildren() == N);
  for (auto & child : *dnode)
    assert(children.find(child->node()) != children.end());
}

static const jlm::llvm::DominatorTreeNode *
get_child(const jlm::llvm::DominatorTreeNode * root, const jlm::llvm::ControlFlowGraphNode * node)
{
  for (const auto & child : *root)
  {
    if (child->node() == node)
      return child.get();
  }

  assert(0);
}

static void
test()
{
  using namespace jlm::llvm;

  InterProceduralGraphModule im(jlm::util::FilePath(""), "", "");

  /* setup cfg */

  ControlFlowGraph cfg(im);
  auto bb1 = BasicBlock::create(cfg);
  auto bb2 = BasicBlock::create(cfg);
  auto bb3 = BasicBlock::create(cfg);
  auto bb4 = BasicBlock::create(cfg);

  cfg.exit()->divert_inedges(bb1);
  bb1->add_outedge(bb2);
  bb1->add_outedge(bb3);
  bb2->add_outedge(bb3);
  bb2->add_outedge(bb4);
  bb3->add_outedge(bb4);
  bb4->add_outedge(cfg.exit());

  /* verify domtree */

  auto root = domtree(cfg);
  check<1>(root.get(), cfg.entry(), { bb1 });

  auto dtbb1 = root->child(0);
  check<3>(dtbb1, bb1, { bb2, bb3, bb4 });

  auto dtbb2 = get_child(dtbb1, bb2);
  check<0>(dtbb2, bb2, {});

  auto dtbb3 = get_child(dtbb1, bb3);
  check<0>(dtbb3, bb3, {});

  auto dtbb4 = get_child(dtbb1, bb4);
  check<1>(dtbb4, bb4, { cfg.exit() });

  auto dtexit = dtbb4->child(0);
  check<0>(dtexit, cfg.exit(), {});
}

JLM_UNIT_TEST_REGISTER("jlm/llvm/ir/test-domtree", test)
