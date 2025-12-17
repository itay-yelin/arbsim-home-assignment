#include "SimulationEngine.h"
#include <ostream>

namespace ArbSim {

    SimulationEngine::SimulationEngine(Strategy strategy, PnlTracker pnl, std::ostream& tradeLog)
        : strategy_(std::move(strategy))
        , pnl_(std::move(pnl))
        , tradeLog_(tradeLog)
    {
    }

    void SimulationEngine::OnQuoteA(const MarketEvent& ev)
    {
        lastQuoteA_ = ev;
    }

    void SimulationEngine::OnQuoteB(const MarketEvent& ev)
    {
        lastQuoteB_ = ev;
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

        tradeLog_
            << time
            << "," << (side == Side::Buy ? "BUY" : "SELL")
            << ",FutureB," << qty
            << "," << mid
            << "," << reasonTag
            << "\n";
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

        if (!lastQuoteA_.has_value() || !lastQuoteB_.has_value())
        {
            return;
        }

        const auto& a = *lastQuoteA_;
        const auto& b = *lastQuoteB_;

        const double sellEdge = b.bid - a.ask; // sell B at bid, buy A at ask
        const double buyEdge = a.bid - b.ask; // sell A at bid, buy B at ask

        StrategyAction action =
            strategy_.Decide(sellEdge, buyEdge, pnl_.GetPositionB(), pnl_.GetTotalPnl());


        if (action == StrategyAction::BuyB)
        {
            pnl_.ApplyTradeB(time, Side::Buy, lastQuoteB_->ask, 1);
            tradeLog_ << time << ",BUY,FutureB,1," << lastQuoteB_->ask << "\n";
        }
        else if (action == StrategyAction::SellB)
        {
            pnl_.ApplyTradeB(time, Side::Sell, lastQuoteB_->bid, 1);
            tradeLog_ << time << ",SELL,FutureB,1," << lastQuoteB_->bid << "\n";
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

        if (!lastQuoteA_.has_value() || !lastQuoteB_.has_value())
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
