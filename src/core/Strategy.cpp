#include "Strategy.h"

#include <cmath>
#include <cstdlib>

namespace {
// Epsilon for floating-point comparisons to avoid precision errors
constexpr double kEpsilon = 1e-9;
} // namespace

namespace ArbSim {

Strategy::Strategy(const StrategyParams &params) : params_(params) {}

Strategy::Strategy(const Config &cfg) {
  params_.MinArbitrageEdge = cfg.GetDouble("Strategy.MinArbitrageEdge");
  params_.StopLossPnl = cfg.GetDouble("Strategy.StopLossPnl");
  params_.MaxAbsExposureLots = cfg.GetInt("Strategy.MaxAbsExposureLots");
}

const StrategyParams &Strategy::GetParams() const { return params_; }

StrategyAction Strategy::Decide(double sellEdge, double buyEdge, int positionB,
                                double currentPnl) const {
  // 1. DECISION: Stop Loss (Moved from Engine to Strategy)
  if (currentPnl < params_.StopLossPnl) {
    return StrategyAction::Flatten;
  }

  // 2. DECISION: Entry Logic
  // Use epsilon tolerance to avoid missing trades due to floating-point precision
  if (sellEdge >= params_.MinArbitrageEdge - kEpsilon) {
    const int nextPos = positionB - 1;
    if (std::abs(nextPos) > params_.MaxAbsExposureLots) {
      return StrategyAction::None;
    }
    return StrategyAction::SellB;
  }

  if (buyEdge >= params_.MinArbitrageEdge - kEpsilon) {
    const int nextPos = positionB + 1;
    if (std::abs(nextPos) > params_.MaxAbsExposureLots) {
      return StrategyAction::None;
    }
    return StrategyAction::BuyB;
  }

  return StrategyAction::None;
}

} // namespace ArbSim