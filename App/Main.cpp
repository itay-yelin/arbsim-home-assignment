#include <iostream>
#include <optional>

#include "CsvReader.h"
#include "StreamMerger.h"
#include "MarketData.h"
#include "PnlTracker.h"

using namespace ArbSim;


static double CalcMid(const MarketEvent& ev)
{
    return (ev.bid + ev.ask) * 0.5;
}


int main()
{
    try
    {
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

            // TODO (next steps):
            // 1. Decide action based on edge and parameters:
            //    MinArbEdge, MaxAbsExposure, StopLossPnl
            // 2. If Buy:  pnl.ApplyTradeB(event.sendingTime, Side::Buy,  lastQuoteB->ask, 1);
            // 3. If Sell: pnl.ApplyTradeB(event.sendingTime, Side::Sell, lastQuoteB->bid, 1);
            // 4. Stop-loss check and FlattenAtMid
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
