#ifndef STRATEGY_PARAMS_H
#define STRATEGY_PARAMS_H

#include <stdexcept>
#include <string>

namespace ArbSim {

struct StrategyParams {
  double MinArbitrageEdge;
  double StopLossPnl;
  int MaxAbsExposureLots;

  // Validates parameters are within acceptable ranges
  void Validate() const {
    if (MinArbitrageEdge < 0.0) {
      throw std::invalid_argument(
          "StrategyParams: MinArbitrageEdge must be >= 0, got " +
          std::to_string(MinArbitrageEdge));
    }
    if (MaxAbsExposureLots < 1) {
      throw std::invalid_argument(
          "StrategyParams: MaxAbsExposureLots must be >= 1, got " +
          std::to_string(MaxAbsExposureLots));
    }
    if (StopLossPnl > 0.0) {
      throw std::invalid_argument(
          "StrategyParams: StopLossPnl must be <= 0 (it's a loss threshold), got " +
          std::to_string(StopLossPnl));
    }
  }
};

} // namespace ArbSim

#endif // STRATEGY_PARAMS_H