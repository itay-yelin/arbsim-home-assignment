#ifndef I_STRATEGY_H
#define I_STRATEGY_H

#include "StrategyParams.h"

namespace ArbSim {

enum class StrategyAction {
  None,
  BuyB,
  SellB,
  Flatten, // Close all positions at mid price
};

class IStrategy {
public:
  virtual ~IStrategy() = default;

  virtual const StrategyParams &GetParams() const = 0;

  virtual StrategyAction Decide(double sellEdge, double buyEdge, int positionB,
                                double currentPnl) const = 0;
};

} // namespace ArbSim

#endif // I_STRATEGY_H