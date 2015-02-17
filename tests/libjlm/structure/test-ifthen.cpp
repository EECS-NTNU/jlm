/*
 * Copyright 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-registry.hpp"

#include <jlm/IR/clg.hpp>

#include <assert.h>

static int
verify(jlm::frontend::clg & clg)
{
	assert(clg.nnodes() == 1);

	jlm::frontend::clg_node * node = clg.lookup_function("max");
	assert(node != nullptr);

	jlm::frontend::cfg * cfg = node->cfg();
//	jive_cfg_view(cfg);

	assert(cfg->is_structured());

	return 0;
}

JLM_UNIT_TEST_REGISTER("libjlm/structure/test-ifthen", verify);
