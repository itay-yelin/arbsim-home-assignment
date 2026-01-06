#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>
#include <cstdio>

#include "MarketData.h"
#include "PnlTracker.h"
#include "IStrategy.h" // For StrategyAction enum

namespace ArbSim {

    template <typename TStrategy>
    class SimulationEngine {
    public:
        SimulationEngine(TStrategy strategy, PnlTracker pnl,
            std::string& tradeLogBuffer)
            : strategy_(std::move(strategy)), pnl_(std::move(pnl)),
            tradeLog_(tradeLogBuffer), lastQuoteA_{}, lastQuoteB_{},
            stopTrading_(false), hasA_(false), hasB_(false) {
        }

        /**
         * Main entry point for market events.
         * Marked inline to encourage the compiler to embed this in the Main.cpp loop.
         */
        inline void OnEvent(const MarketEvent& ev) {
            if (ev.instrumentId == InstrumentId::FutureA) {
                lastQuoteA_ = ev;
                hasA_ = true;
            }
            else if (ev.instrumentId == InstrumentId::FutureB) {
                lastQuoteB_ = ev;
                hasB_ = true;
                pnl_.OnQuoteB(ev);
            }

            if (!hasA_ || !hasB_) {
                return;
            }

            TryTrade(ev.sendingTime);
        }

        void OnEndOfDay(long long time) {
            if (!stopTrading_) {
                ClosePositionAtMidAsTrade(time, "EOD_CLOSE");
            }
        }

        void PrintSummary(std::ostream& out) const {
            out << "Simulation finished\n";
            out << "Total PnL: " << pnl_.GetTotalPnl() << "\n";
            out << "Best PnL: " << pnl_.GetBestPnl() << "\n";
            out << "Worst PnL: " << pnl_.GetWorstPnl() << "\n";
            out << "Max exposure: " << pnl_.GetMaxAbsExposure() << "\n";
            out << "Traded lots: " << pnl_.GetTradedLots() << "\n";
            out << "Dropped buys: " << droppedBuyCount_ << "\n";
            out << "Dropped sells: " << droppedSellCount_ << "\n";
        }

        // Getters for Main loop logging
        double GetTotalPnl() const { return pnl_.GetTotalPnl(); }
        double GetLastMidB() const { return pnl_.GetLastMidB(); }
        double GetLastMidA() const { return 0.0; }
        bool IsStopped() const { return stopTrading_; }

        // Observability: expose dropped trade counts
        size_t GetDroppedBuyCount() const { return droppedBuyCount_; }
        size_t GetDroppedSellCount() const { return droppedSellCount_; }
        size_t GetTotalDroppedTrades() const { return droppedBuyCount_ + droppedSellCount_; }

    private:
        TStrategy strategy_; 
        PnlTracker pnl_;
        std::string& tradeLog_;

        MarketEvent lastQuoteA_;
        MarketEvent lastQuoteB_;

        bool stopTrading_;
        bool hasA_;
        bool hasB_;

        // Observability: track dropped trades
        size_t droppedBuyCount_ = 0;
        size_t droppedSellCount_ = 0;

        /**
         * Core trading logic. By using a template, strategy_.Decide() is a direct
         * call that the compiler can inline.
         */
        inline void TryTrade(long long time) {
            if (stopTrading_) {
                return;
            }

            const double sellEdge = lastQuoteB_.bid - lastQuoteA_.ask;
            const double buyEdge = lastQuoteA_.bid - lastQuoteB_.ask;
           
            StrategyAction action = strategy_.Decide(
                sellEdge, buyEdge, pnl_.GetPositionB(), pnl_.GetTotalPnl());

            switch (action) {
            case StrategyAction::Flatten: {
                ClosePositionAtMidAsTrade(time, "STOP_LOSS_CLOSE");
                stopTrading_ = true;
                break;
            }
            case StrategyAction::BuyB: {
                if (lastQuoteB_.askSize < 1) {
                    ++droppedBuyCount_;
                    return;
                }
                pnl_.ApplyTradeB(time, Side::Buy, lastQuoteB_.ask, 1);
                LogTrade(time, "BUY", lastQuoteB_.ask);
                break;
            }
            case StrategyAction::SellB: {
                if (lastQuoteB_.bidSize < 1) {
                    ++droppedSellCount_;
                    return;
                }
                pnl_.ApplyTradeB(time, Side::Sell, lastQuoteB_.bid, 1);
                LogTrade(time, "SELL", lastQuoteB_.bid);
                break;
            }
            case StrategyAction::None:
            default:
                break;
            }
        }

        void ClosePositionAtMidAsTrade(long long time, const char* reasonTag) {
            const int pos = pnl_.GetPositionB();
            if (pos == 0 || !pnl_.HasMidB()) return;

            const double mid = pnl_.GetLastMidB();
            const int qty = std::abs(pos);
            const Side side = (pos > 0) ? Side::Sell : Side::Buy;

            pnl_.ApplyTradeB(time, side, mid, qty);

            char buf[160];
            const int n = std::snprintf(buf, sizeof(buf), "%lld,%s,FutureB,%d,%.10g,%s",
                time, (side == Side::Buy ? "BUY" : "SELL"),
                qty, mid, reasonTag);
            if (n > 0) AppendLogLine(buf);
        }

        void LogTrade(long long time, const char* side, double price) {
            char buf[128];
            const int n = std::snprintf(buf, sizeof(buf), "%lld,%s,FutureB,1,%.10g",
                time, side, price);
            if (n > 0) AppendLogLine(buf);
        }

        void AppendLogLine(const char* line) {
            tradeLog_.append(line);
            tradeLog_.push_back('\n');
        }
    };

} // namespace ArbSim

#endif // SIMULATION_ENGINE_H