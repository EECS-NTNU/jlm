/*
 * Copyright 2021 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_HLS_BACKEND_RVSDG2RHLS_THETACONVERSION_HPP
#define JLM_HLS_BACKEND_RVSDG2RHLS_THETACONVERSION_HPP

#include <jlm/rvsdg/Transformation.hpp>

namespace jlm::hls
{

/**
 * \brief Converts theta nodes to HLS loop structures.
 *
 * Theta nodes in the RVSDG represent loops with carry variables. This transformation converts
 * them to HLS LoopNode representations suitable for hardware synthesis and FIRRTL generation.
 */
class ThetaNodeConversion final : public rvsdg::Transformation
{
public:
  ~ThetaNodeConversion() noexcept override;

  ThetaNodeConversion();

  ThetaNodeConversion(const ThetaNodeConversion &) = delete;

  ThetaNodeConversion &
  operator=(const ThetaNodeConversion &) = delete;

  void
  Run(rvsdg::RvsdgModule & rvsdgModule, util::StatisticsCollector & statisticsCollector) override;

  /**
   * \brief Creates and runs a theta node conversion transformation.
   */
  static void
  CreateAndRun(rvsdg::RvsdgModule & rvsdgModule, util::StatisticsCollector & statisticsCollector)
  {
    ThetaNodeConversion thetaNodeConversion;
    thetaNodeConversion.Run(rvsdgModule, statisticsCollector);
  }
};

}

#endif // JLM_HLS_BACKEND_RVSDG2RHLS_THETACONVERSION_HPP