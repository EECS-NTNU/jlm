/*
 * Copyright 2021 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_HLS_BACKEND_RVSDG2RHLS_BRAM_POINTERS_HPP
#define JLM_HLS_BACKEND_RVSDG2RHLS_BRAM_POINTERS_HPP

#include <jlm/llvm/ir/RvsdgModule.hpp>
#include <jlm/rvsdg/region.hpp>

namespace jlm::hls
{
void
bram_pointers(rvsdg::Region * region);

void
bram_pointers(jlm::llvm::RvsdgModule & rm);
}

#endif // JLM_HLS_BACKEND_RVSDG2RHLS_BRAM_POINTERS_HPP
