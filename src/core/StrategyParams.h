#ifndef STRATEGY_PARAMS_H
#define STRATEGY_PARAMS_H

namespace ArbSim {

struct StrategyParams {
  double MinArbitrageEdge;
  double StopLossPnl;
  int MaxAbsExposureLots;
};

} // namespace ArbSim

#endif // STRATEGY_PARAMS_H