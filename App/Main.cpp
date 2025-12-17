#include <iostream>

#include "CsvReader.h"
#include "StreamMerger.h"
#include "MarketData.h"
#include "PnlTracker.h"

using namespace ArbSim;

int main()
{
    try
    {
        CsvReader readerA("Data/FutureA.csv");
        CsvReader readerB("Data/FutureB.csv");

        StreamMerger merger(readerA, readerB);

        PnlTracker pnl;

        MarketEvent event;

        while (merger.ReadNext(event))
        {
            if (event.instrumentId == InstrumentId::FutureB)
            {
                pnl.OnQuoteB(event);
            }

            // TODO (next steps):
            // 1. Update last quote A
            // 2. Compute arbitrage edge
            // 3. Decide Buy / Sell / None
            // 4. ApplyTradeB(...)
            // 5. Stop-loss check
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
