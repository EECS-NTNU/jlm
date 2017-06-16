/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_RVSDG2JLM_RVSDG2JLM_HPP
#define JLM_RVSDG2JLM_RVSDG2JLM_HPP

#include <memory>

namespace jive {

class graph;

}

namespace jlm {

class module;

namespace rvsdg2jlm {

std::unique_ptr<jlm::module>
rvsdg2jlm(const jive::graph & graph);

}}

#endif