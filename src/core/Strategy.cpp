#include "Strategy.h"

#include <cmath>

namespace ArbSim
{

    Strategy::Strategy(const StrategyParams& params)
        : params_(params)
    {
    }

    const StrategyParams& Strategy::GetParams() const
    {
        return params_;
    }

    StrategyAction Strategy::Decide(
        double sellEdge,
        double buyEdge,
        int positionB,
        double currentPnl
    ) const
    {
        if (currentPnl < params_.StopLossPnl)
        {
            return StrategyAction::None;
        }

        if (sellEdge >= params_.MinArbitrageEdge)
        {
            // exposure check fixed below in section 2
            const int nextPos = positionB - 1;
            if (std::abs(nextPos) > params_.MaxAbsExposureLots)
            {
                return StrategyAction::None;
            }

            return StrategyAction::SellB;
        }

        if (buyEdge >= params_.MinArbitrageEdge)
        {
            const int nextPos = positionB + 1;
            if (std::abs(nextPos) > params_.MaxAbsExposureLots)
            {
                return StrategyAction::None;
            }

            return StrategyAction::BuyB;
        }

        return StrategyAction::None;
    }

} // namespace ArbSim
