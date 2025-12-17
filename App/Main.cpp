#include <iostream>
#include <optional>

#include "Config.h"
#include "CsvReader.h"
#include "StreamMerger.h"
#include "MarketData.h"
#include "PnlTracker.h"
#include "Strategy.h"
#include "SimulationEngine.h"

using namespace ArbSim;
int main()
{
    try
    {
        Config cfg("config.cfg");

        CsvReader readerA(cfg.GetString("Data.FutureA"));
        CsvReader readerB(cfg.GetString("Data.FutureB"));

        StreamMerger merger(readerA, readerB);

        StrategyParams params;
        params.MinArbitrageEdge = cfg.GetDouble("Strategy.MinArbitrageEdge");
        params.MaxAbsExposureLots = cfg.GetInt("Strategy.MaxAbsExposureLots");
        params.StopLossPnl = cfg.GetDouble("Strategy.StopLossPnl");

        Strategy strategy(params);
        PnlTracker pnl;

        SimulationEngine engine(strategy, pnl, std::cout);

        MarketEvent ev{};
        long long lastTime = 0;

        while (merger.ReadNext(ev))
        {
            lastTime = ev.sendingTime;
            engine.OnEvent(ev);
        }

        engine.OnEndOfDay(lastTime);
        engine.PrintSummary(std::cout);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
