#include <iostream>
#include <optional>

#include "CsvReader.h"
#include "StreamMerger.h"
#include "MarketData.h"
#include "PnlTracker.h"
#include "Strategy.h"

using namespace ArbSim;


static double CalcMid(const MarketEvent& ev)
{
    return (ev.bid + ev.ask) * 0.5;
}


int main()
{
    try
    {
        StrategyParams params;
        params.MinArbitrageEdge = 1.0;
        params.MaxAbsExposureLots = 2;
        params.StopLossPnl = -50.0;

        Strategy strategy(params);
        bool stopTrading = false;

        CsvReader readerA("Data/FutureA.csv");
        CsvReader readerB("Data/FutureB.csv");

        StreamMerger merger(readerA, readerB);

        PnlTracker pnl;

        std::optional<MarketEvent> lastQuoteA;
        std::optional<MarketEvent> lastQuoteB;

        MarketEvent event;

        while (merger.ReadNext(event))
        {
            if (event.instrumentId == InstrumentId::FutureA)
            {
                lastQuoteA = event;
            }
            else if (event.instrumentId == InstrumentId::FutureB)
            {
                lastQuoteB = event;
                pnl.OnQuoteB(event);
            }
            if (!lastQuoteA.has_value() || !lastQuoteB.has_value())
            {
                continue;
            }

            const double midA = CalcMid(*lastQuoteA);
            const double midB = CalcMid(*lastQuoteB);

            const double edge = midB - midA;

            if (!stopTrading && pnl.GetTotalPnl() < params.StopLossPnl)
            {
                pnl.FlattenAtMid(event.sendingTime);
                std::cout << event.sendingTime << ",FLATTEN,FutureB," << pnl.GetLastMidB() << std::endl;
                stopTrading = true;
            }

            if (stopTrading)
            {
                continue;
            }

            StrategyAction action = strategy.Decide(midA, midB, pnl.GetPositionB(), pnl.GetTotalPnl());

            if (action == StrategyAction::BuyB)
            {
                pnl.ApplyTradeB(event.sendingTime, Side::Buy, lastQuoteB->ask, 1);
                std::cout << event.sendingTime << ",BUY,FutureB,1," << lastQuoteB->ask << std::endl;
            }
            else if (action == StrategyAction::SellB)
            {
                pnl.ApplyTradeB(event.sendingTime, Side::Sell, lastQuoteB->bid, 1);
                std::cout << event.sendingTime << ",SELL,FutureB,1," << lastQuoteB->bid << std::endl;
            }
        }

        std::cout << "Simulation finished" << std::endl;
        std::cout << "Total PnL: " << pnl.GetTotalPnl() << std::endl;
        std::cout << "Best PnL: " << pnl.GetBestPnl() << std::endl;
        std::cout << "Worst PnL: " << pnl.GetWorstPnl() << std::endl;
        std::cout << "Max exposure: " << pnl.GetMaxAbsExposure() << std::endl;
        std::cout << "Traded lots: " << pnl.GetTradedLots() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
