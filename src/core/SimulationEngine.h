#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H
#include <iosfwd>

#include "MarketData.h"
#include "PnlTracker.h"
#include "Strategy.h"

namespace ArbSim {

    class SimulationEngine
    {
    public:
        SimulationEngine(Strategy strategy, PnlTracker pnl, std::string& tradeLogBuffer);

        void OnEvent(const MarketEvent& ev);
        void OnEndOfDay(long long time);

        void PrintSummary(std::ostream& out) const;

    private:
        void OnQuoteA(const MarketEvent& ev);
        void OnQuoteB(const MarketEvent& ev);

        void TryTrade(long long time);
        void CheckStopLossAndMaybeFlatten(long long time);
        void ClosePositionAtMidAsTrade(long long time, const char* reasonTag);
        
        void AppendLogLine(const char* line);

    public:
        // Returns true if trading has been stopped (e.g. by stop loss or end of day)
        bool IsStopped() const { 
            return stopTrading_; 
        }

        double GetTotalPnl() const { return pnl_.GetTotalPnl(); }
        double GetLastMidB() const { return pnl_.GetLastMidB(); }
        double GetLastMidA() const { return (lastQuoteA_.bid + lastQuoteA_.ask) * 0.5; }

        
    private:
        Strategy strategy_;
        PnlTracker pnl_;
        std::string& tradeLog_;

        bool stopTrading_ = false;
        MarketEvent lastQuoteA_;
        MarketEvent lastQuoteB_;
        bool hasA_ = false;
        bool hasB_ = false;
    };

} // namespace ArbSim

#endif // SIMULATION_ENGINE_H
