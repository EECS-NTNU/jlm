/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-operation.hpp"
#include "test-registry.hpp"
#include "test-types.hpp"

#include <jlm/ir/basic_block.hpp>
#include <jlm/ir/cfg.hpp>
#include <jlm/ir/module.hpp>
#include <jlm/ir/operators.hpp>
#include <jlm/ir/ssa.hpp>
#include <jlm/ir/view.hpp>

static inline void
test_two_phis()
{
	jlm::valuetype vt;
	jlm::module module("", "");

	auto v1 = module.create_variable(vt, "vbl1", false);
	auto v2 = module.create_variable(vt, "vbl2", false);
	auto v3 = module.create_variable(vt, "vbl3", false);
	auto v4 = module.create_variable(vt, "vbl4", false);

	auto r1 = module.create_variable(vt, "r1", false);
	auto r2 = module.create_variable(vt, "r2", false);

	jlm::cfg cfg(module);
	auto bb1 = create_basic_block_node(&cfg);
	auto bb2 = create_basic_block_node(&cfg);
	auto bb3 = create_basic_block_node(&cfg);
	auto bb4 = create_basic_block_node(&cfg);

	cfg.exit_node()->divert_inedges(bb1);
	bb1->add_outedge(bb2);
	bb1->add_outedge(bb3);
	bb2->add_outedge(bb4);
	bb3->add_outedge(bb4);
	bb4->add_outedge(cfg.exit_node());

	append_last(bb2, jlm::create_testop_tac({}, {v1}));
	append_last(bb2, jlm::create_testop_tac({}, {v3}));
	append_last(bb3, jlm::create_testop_tac({}, {v2}));
	append_last(bb3, jlm::create_testop_tac({}, {v4}));

	append_last(bb4, jlm::create_phi_tac({{v1, bb2}, {v2, bb3}}, r1));
	append_last(bb4, jlm::create_phi_tac({{v3, bb2}, {v4, bb3}}, r2));

//	jlm::view_ascii(cfg, stdout);

	jlm::destruct_ssa(cfg);

//	jlm::view_ascii(cfg, stdout);
}

static int
verify()
{
	test_two_phis();

	return 0;
}

JLM_UNIT_TEST_REGISTER("libjlm/test-ssa-destruction", verify);
