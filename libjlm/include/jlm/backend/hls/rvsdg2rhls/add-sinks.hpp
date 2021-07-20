/*
 * Copyright 2021 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_BACKEND_HLS_RVSDG2RHLS_ADD_SINKS_HPP
#define JLM_BACKEND_HLS_RVSDG2RHLS_ADD_SINKS_HPP

#include <jive/rvsdg/region.hpp>
#include <jlm/ir/RvsdgModule.hpp>

namespace jlm{
	namespace hls{
		void
		add_sinks(jive::region *region);

		void
		add_sinks(jlm::RvsdgModule &rm);
	}
}
#endif //JLM_BACKEND_HLS_RVSDG2RHLS_ADD_SINKS_HPP
