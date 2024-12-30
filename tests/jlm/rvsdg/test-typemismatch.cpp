/*
 * Copyright 2010 2011 2012 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-operation.hpp"
#include "test-registry.hpp"
#include "test-types.hpp"

static int
test_main()
{
  using namespace jlm::rvsdg;

  Graph graph;

  auto type = jlm::tests::statetype::Create();
  auto value_type = jlm::tests::valuetype::Create();

  auto n1 = jlm::tests::test_op::create(&graph.GetRootRegion(), {}, { type });

  bool error_handler_called = false;
  try
  {
    jlm::tests::test_op::Create(&graph.GetRootRegion(), { value_type }, { n1->output(0) }, {});
  }
  catch (jlm::util::type_error & e)
  {
    error_handler_called = true;
  }

  assert(error_handler_called);

  return 0;
}

JLM_UNIT_TEST_REGISTER("jlm/rvsdg/test-typemismatch", test_main)
