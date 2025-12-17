#ifndef STRATEGY_H
#define STRATEGY_H

namespace ArbSim
{

    enum class StrategyAction
    {
        None,
        BuyB,
        SellB
    };

    struct StrategyParams
    {
        double MinArbitrageEdge;
        int MaxAbsExposureLots;
        double StopLossPnl;
    };

    class Strategy
    {
    public:
        explicit Strategy(const StrategyParams& params);

        StrategyAction Decide(
            double sellEdge,   // B_bid - A_ask
            double buyEdge,    // A_bid - B_ask
            int positionB,
            double currentPnl
        ) const;

        const StrategyParams& GetParams() const;

    private:
        StrategyParams params_;
    };

} // namespace ArbSim

#endif // STRATEGY_H
