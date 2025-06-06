/*
 * Copyright 2024 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_BACKEND_HLS_UTIL_VIEW_HPP
#define JLM_BACKEND_HLS_UTIL_VIEW_HPP

#include <jlm/llvm/ir/RvsdgModule.hpp>
#include <jlm/rvsdg/graph.hpp>
#include <string>

namespace jlm::hls
{

enum ViewColors
{
  NONE,
  BLACK,
  RED
};

std::string
ViewcolorToString(const ViewColors & color);

std::string
RegionToDot(
    rvsdg::Region * region,
    std::unordered_map<rvsdg::Output *, ViewColors> & outputColor,
    std::unordered_map<rvsdg::Input *, ViewColors> & inputColor,
    std::unordered_map<rvsdg::Output *, ViewColors> & tailLabel);

std::string
ToDot(
    rvsdg::Region * region,
    std::unordered_map<rvsdg::Output *, ViewColors> & outputColor,
    std::unordered_map<rvsdg::Input *, ViewColors> & inputColor,
    std::unordered_map<rvsdg::Output *, ViewColors> & tailLabel);

void
ViewDot(rvsdg::Region * region, FILE * out);

void
ViewDot(
    rvsdg::Region * region,
    FILE * out,
    std::unordered_map<rvsdg::Output *, ViewColors> & outputColor,
    std::unordered_map<rvsdg::Input *, ViewColors> & inputColor,
    std::unordered_map<rvsdg::Output *, ViewColors> & tailLabel);

void
DumpDot(
    llvm::RvsdgModule & rvsdgModule,
    const std::string & fileName,
    std::unordered_map<rvsdg::Output *, ViewColors> outputColor,
    std::unordered_map<rvsdg::Input *, ViewColors> inputColor,
    std::unordered_map<rvsdg::Output *, ViewColors> tailLabel);
void

DumpDot(llvm::RvsdgModule & rvsdgModule, const std::string & fileName);

void
DumpDot(
    rvsdg::Region * region,
    const std::string & fileName,
    std::unordered_map<rvsdg::Output *, ViewColors> outputColor,
    std::unordered_map<rvsdg::Input *, ViewColors> inputColor,
    std::unordered_map<rvsdg::Output *, ViewColors> tailLabel);
void
DumpDot(rvsdg::Region * region, const std::string & fileName);

void
DotToSvg(const std::string & fileName);

} // namespace jlm::hls

#endif // JLM_BACKEND_HLS_UTIL_VIEW_HPP
