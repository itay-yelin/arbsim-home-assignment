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
        double midA,
        double midB,
        int positionB,
        double currentPnl
    ) const
    {
        if (currentPnl < params_.StopLossPnl)
        {
            return StrategyAction::None;
        }

        const double edge = midB - midA;

        if (edge >= params_.MinArbitrageEdge)
        {
            if (std::abs(positionB) >= params_.MaxAbsExposureLots)
            {
                return StrategyAction::None;
            }

            return StrategyAction::SellB;
        }

        if (edge <= -params_.MinArbitrageEdge)
        {
            if (std::abs(positionB) >= params_.MaxAbsExposureLots)
            {
                return StrategyAction::None;
            }

            return StrategyAction::BuyB;
        }

        return StrategyAction::None;
    }

} // namespace ArbSim
