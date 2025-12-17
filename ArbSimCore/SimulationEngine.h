#pragma once
#include <optional>
#include <iosfwd>

#include "MarketData.h"
#include "PnlTracker.h"
#include "Strategy.h"

namespace ArbSim {

    class SimulationEngine
    {
    public:
        SimulationEngine(Strategy strategy, PnlTracker pnl, std::ostream& tradeLog);

        void OnEvent(const MarketEvent& ev);
        void OnEndOfDay(long long time);

        void PrintSummary(std::ostream& out) const;

    private:
        static double CalcMid(const MarketEvent& ev);

        void OnQuoteA(const MarketEvent& ev);
        void OnQuoteB(const MarketEvent& ev);

        void TryTrade(long long time);
        void CheckStopLossAndMaybeFlatten(long long time);

    private:
        Strategy strategy_;
        PnlTracker pnl_;
        std::ostream& tradeLog_;

        bool stopTrading_ = false;
        std::optional<MarketEvent> lastQuoteA_;
        std::optional<MarketEvent> lastQuoteB_;
    };

} // namespace ArbSim
#pragma once
