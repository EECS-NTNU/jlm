#ifndef JLM_HLS_BACKEND_RVSDG2RHLS_GAMMACONVERSION_HPP
#define JLM_HLS_BACKEND_RVSDG2RHLS_GAMMACONVERSION_HPP

#include <jlm/rvsdg/Transformation.hpp>

namespace jlm::hls
{

/**
 * \brief Converts gamma nodes to HLS multiplexer operations.
 *
 * Gamma nodes in the RVSDG represent conditional control flow but need to be converted to
 * HLS-specific mux operations for hardware synthesis. This transformation is required before
 * FIRRTL generation and must run after gamma node creation but before the R-HLS conversion.
 *
 * \see RhlsToFirrtlConverter The FIRRTL converter that expects converted gamma nodes.
 */
class GammaNodeConversion final : public rvsdg::Transformation
{
public:
  ~GammaNodeConversion() noexcept override;

  GammaNodeConversion();

  GammaNodeConversion(const GammaNodeConversion &) = delete;

  GammaNodeConversion &
  operator=(const GammaNodeConversion &) = delete;

  void
  Run(rvsdg::RvsdgModule & rvsdgModule, util::StatisticsCollector & statisticsCollector) override;

  /**
   * \brief Creates and runs a gamma node conversion transformation.
   *
   * This static helper creates an instance of GammaNodeConversion and runs it on the
   * provided RVSDG module.
   */
  static void
  CreateAndRun(rvsdg::RvsdgModule & rvsdgModule, util::StatisticsCollector & statisticsCollector)
  {
    GammaNodeConversion gammaNodeConversion;
    gammaNodeConversion.Run(rvsdgModule, statisticsCollector);
  }
};

}

#endif // JLM_HLS_BACKEND_RVSDG2RHLS_GAMMACONVERSION_HPP