#include "SimulationEngine.h"
#include <ostream>

namespace ArbSim {

    SimulationEngine::SimulationEngine(Strategy strategy, PnlTracker pnl, std::ostream& tradeLog)
        : strategy_(std::move(strategy))
        , pnl_(std::move(pnl))
        , tradeLog_(tradeLog)
    {
    }

    double SimulationEngine::CalcMid(const MarketEvent& ev)
    {
        return (ev.bid + ev.ask) * 0.5;
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

    void SimulationEngine::CheckStopLossAndMaybeFlatten(long long time)
    {
        if (stopTrading_)
            return;

        const auto& params = strategy_.GetParams();
        if (pnl_.GetTotalPnl() < params.StopLossPnl)
        {
            pnl_.FlattenAtMid(time);
            tradeLog_ << time << ",FLATTEN,FutureB,0," << pnl_.GetLastMidB() << "\n";
            stopTrading_ = true;
        }
    }

    void SimulationEngine::TryTrade(long long time)
    {
        if (stopTrading_)
            return;

        if (!lastQuoteA_.has_value() || !lastQuoteB_.has_value())
            return;

        const double midA = CalcMid(*lastQuoteA_);
        const double midB = CalcMid(*lastQuoteB_);

        StrategyAction action =
            strategy_.Decide(midA, midB, pnl_.GetPositionB(), pnl_.GetTotalPnl());

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
            OnQuoteA(ev);
        else if (ev.instrumentId == InstrumentId::FutureB)
            OnQuoteB(ev);

        if (!lastQuoteA_.has_value() || !lastQuoteB_.has_value())
            return;

        CheckStopLossAndMaybeFlatten(ev.sendingTime);
        if (stopTrading_)
            return;

        TryTrade(ev.sendingTime);
    }

    void SimulationEngine::OnEndOfDay(long long time)
    {
        // Optional: if you want to ensure flat at end of simulation.
        // Keep or delete depending on spec interpretation.
        if (!stopTrading_ && pnl_.GetPositionB() != 0 && pnl_.HasMidB())
        {
            pnl_.FlattenAtMid(time);
            tradeLog_ << time << ",FLATTEN_EOD,FutureB,0," << pnl_.GetLastMidB() << "\n";
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
