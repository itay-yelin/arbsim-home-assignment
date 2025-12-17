#include "SimulationEngine.h"
#include <ostream>
#include <cstdio>

namespace ArbSim {
    SimulationEngine::SimulationEngine(Strategy strategy, PnlTracker pnl, std::string& tradeLogBuffer)
        : strategy_(std::move(strategy))
        , pnl_(std::move(pnl))
        , tradeLog_(tradeLogBuffer)
    {
    }

    void SimulationEngine::AppendLogLine(const char* line)
    {
        tradeLog_.append(line);
        tradeLog_.push_back('\n');
    }

    void SimulationEngine::OnQuoteA(const MarketEvent& ev)
    {
        lastQuoteA_ = ev;
		hasA_ = true;
    }

    void SimulationEngine::OnQuoteB(const MarketEvent& ev)
    {
        lastQuoteB_ = ev;
		hasB_ = true;

        pnl_.OnQuoteB(ev);
    }

    void SimulationEngine::ClosePositionAtMidAsTrade(long long time, const char* reasonTag)
    {
        const int pos = pnl_.GetPositionB();
        if (pos == 0)
            return;

        if (!pnl_.HasMidB())
        {
            return;
        }

        const double mid = pnl_.GetLastMidB();
        const int qty = (pos > 0) ? pos : -pos;
        const Side side = (pos > 0) ? Side::Sell : Side::Buy;

        // Execute close as a real trade so it:
        // 1) is logged as BUY/SELL
        // 2) increments tradedLots_
        // 3) updates max exposure and PnL consistently
        pnl_.ApplyTradeB(time, side, mid, qty);

        char buf[160];
        const int n = std::snprintf(
            buf, sizeof(buf),
            "%lld,%s,FutureB,%d,%.10g,%s",
            time,
            (side == Side::Buy ? "BUY" : "SELL"),
            qty,
            mid,
            reasonTag
        );

        if (n > 0) 
        { 
            tradeLog_.append(buf, buf + n);
            tradeLog_.push_back('\n');
        }
    }
    void SimulationEngine::CheckStopLossAndMaybeFlatten(long long time)
    {
        if (stopTrading_)
        {
            return;
        }

        const auto& params = strategy_.GetParams();
        if (pnl_.GetTotalPnl() < params.StopLossPnl)
        {
            ClosePositionAtMidAsTrade(time, "STOP_LOSS_CLOSE");
            stopTrading_ = true;
        }
    }

    void SimulationEngine::TryTrade(long long time)
    {
        if (stopTrading_)
        {
            return;
        }

        if (!hasA_ || !hasB_)
        {
            return;
        }

        const auto& a = lastQuoteA_;
        const auto& b = lastQuoteB_;

        const double sellEdge = b.bid - a.ask; // sell B at bid, buy A at ask
        const double buyEdge = a.bid - b.ask; // sell A at bid, buy B at ask

        StrategyAction action =
            strategy_.Decide(sellEdge, buyEdge, pnl_.GetPositionB(), pnl_.GetTotalPnl());


        if (action == StrategyAction::BuyB)
        {
            if (b.askSize < 1)
            {
                return; // no liquidity on ask
            }

            pnl_.ApplyTradeB(time, Side::Buy, b.ask, 1);
            char buf[128];
            const int n = std::snprintf(buf, sizeof(buf), "%lld,BUY,FutureB,1,%.10g", time, b.ask);
            if (n > 0) 
            {
                tradeLog_.append(buf, buf + n);
                tradeLog_.push_back('\n');
            }

        }
        else if (action == StrategyAction::SellB)
        {
            if (b.bidSize < 1)
            {
                return; // no liquidity on bid
            }

            pnl_.ApplyTradeB(time, Side::Sell, b.bid, 1);

            char buf[128];
            const int n = std::snprintf(buf, sizeof(buf), "%lld,SELL,FutureB,1,%.10g", time, b.bid);
            if (n > 0) 
            { 
                tradeLog_.append(buf, buf + n);
                tradeLog_.push_back('\n');
            }
        }

    }

    void SimulationEngine::OnEvent(const MarketEvent& ev)
    {
        if (ev.instrumentId == InstrumentId::FutureA)
        {
            OnQuoteA(ev);
        }
        else if (ev.instrumentId == InstrumentId::FutureB)
        {
            OnQuoteB(ev);
        }

        if (!hasA_ || !hasB_)
        {
            return;
        }

        CheckStopLossAndMaybeFlatten(ev.sendingTime);
        if (stopTrading_)
        {
            return;
        }

        TryTrade(ev.sendingTime);
    }

    void SimulationEngine::OnEndOfDay(long long time)
    {
        // Close any remaining open position at mid as a real trade.
         // If you decide you do NOT want EOD close, delete this method body.
        if (!stopTrading_)
        {
            ClosePositionAtMidAsTrade(time, "EOD_CLOSE");
        }
    }

    void SimulationEngine::PrintSummary(std::ostream& out) const
    {
        out << "Simulation finished\n";
        out << "Total PnL: " << pnl_.GetTotalPnl() << "\n";
        out << "Best PnL: " << pnl_.GetBestPnl() << "\n";
        out << "Worst PnL: " << pnl_.GetWorstPnl() << "\n";
        out << "Max exposure: " << pnl_.GetMaxAbsExposure() << "\n";
        out << "Traded lots: " << pnl_.GetTradedLots() << "\n";
    }

} // namespace ArbSim
