#include <iostream>
#include <optional>
#include <chrono>

#include "Config.h"
#include "CsvReader.h"
#include "StreamMerger.h"
#include "MarketData.h"
#include "PnlTracker.h"
#include "Strategy.h"
#include "SimulationEngine.h"

using namespace ArbSim;

using Clock = std::chrono::steady_clock;

static double Ms(Clock::time_point a, Clock::time_point b)
{
    return std::chrono::duration<double, std::milli>(b - a).count();
}

static double Sec(Clock::time_point a, Clock::time_point b)
{
    return std::chrono::duration<double>(b - a).count();
}

int main()
{
    try
    {
        const auto t_total0 = Clock::now();
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

        std::uint64_t events = 0;
        double onEventMsSum = 0.0;
        const auto t_loop0 = Clock::now();


        while (merger.ReadNext(ev))
        {
            const auto t0 = Clock::now();
            engine.OnEvent(ev);
            const auto t1 = Clock::now();

            onEventMsSum += Ms(t0, t1);
            ++events;
            if (engine.IsStopped())
            {
                break;
            }
        }
        
        const auto t_loop1 = Clock::now();
        engine.OnEndOfDay(lastTime);
        engine.PrintSummary(std::cout);

        const auto t_total1 = Clock::now();

        const double loopMs = Ms(t_loop0, t_loop1);
        const double totalMs = Ms(t_total0, t_total1);
        const double loopSec = Sec(t_loop0, t_loop1);

        std::cout << "\nTiming\n";
        std::cout << "Events: " << events << "\n";
        std::cout << "Loop time: " << loopMs << " ms\n";
        std::cout << "Total time: " << totalMs << " ms\n";
        std::cout << "Events/sec: " << (loopSec > 0.0 ? (events / loopSec) : 0.0) << "\n";
        std::cout << "OnEvent total: " << onEventMsSum << " ms\n";
        std::cout << "OnEvent avg: " << (events ? (onEventMsSum / events) : 0.0) << " ms\n";
        std::cout << "IO+merge approx: " << (loopMs - onEventMsSum) << " ms\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
