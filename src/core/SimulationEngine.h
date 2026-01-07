#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <iosfwd>
#include <string>
#include <cstddef>

#include "MarketData.h"
#include "PnlTracker.h"
#include "Strategy.h"

namespace ArbSim {

class SimulationEngine {
public:
    SimulationEngine(Strategy strategy, PnlTracker pnl, std::string& tradeLogBuffer);

    void OnEvent(const MarketEvent& ev);
    void OnEndOfDay(long long time);
    void PrintSummary(std::ostream& out) const;

    // Getters for Main loop logging
    double GetTotalPnl() const;
    double GetLastMidB() const;
    double GetLastMidA() const;
    bool IsStopped() const;

    // Observability: expose dropped trade counts
    size_t GetDroppedBuyCount() const;
    size_t GetDroppedSellCount() const;
    size_t GetTotalDroppedTrades() const;

private:
    Strategy strategy_;
    PnlTracker pnl_;
    std::string& tradeLog_;

    MarketEvent lastQuoteA_;
    MarketEvent lastQuoteB_;

    bool stopTrading_;
    bool hasA_;
    bool hasB_;

    size_t droppedBuyCount_;
    size_t droppedSellCount_;

    void TryTrade(long long time);
    void ClosePositionAtMidAsTrade(long long time, const char* reasonTag);
    void LogTrade(long long time, const char* side, double price);
    void AppendLogLine(const char* line);
};

} // namespace ArbSim

#endif // SIMULATION_ENGINE_H
