#ifndef STRATEGY_H
#define STRATEGY_H

#include "../config/Config.h" // Include the Config definition
#include "IStrategy.h"
#include "StrategyParams.h"

namespace ArbSim {

class Strategy : public IStrategy {
public:
  // Option 1: Initialize from a raw struct (Best for Unit Tests)
  explicit Strategy(const StrategyParams &params);

  // Option 2: Initialize directly from Config (Best for Main/Production)
  explicit Strategy(const Config &cfg);

  // -- IStrategy Interface Implementation --
  const StrategyParams &GetParams() const override;

  StrategyAction Decide(double sellEdge, double buyEdge, int positionB,
                        double currentPnl) const override;

private:
  StrategyParams params_;
};

} // namespace ArbSim

#endif // STRATEGY_H