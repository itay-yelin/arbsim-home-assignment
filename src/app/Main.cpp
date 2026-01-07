#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <filesystem>

#include "../config/Config.h"
#include "../core/Constants.h"
#include "../core/CsvReader.h"
#include "../core/MarketData.h"
#include "../core/PnlTracker.h"
#include "../core/SimulationEngine.h"
#include "../core/Strategy.h"
#include "../core/StreamMerger.h"

using namespace ArbSim;

using Clock = std::chrono::steady_clock;

static double Ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

static double Sec(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double>(b - a).count();
}

int main(int argc, char* argv[]) {
    std::cout << "Current Path: " << std::filesystem::current_path() << std::endl;

    try {
        // 1. Optimize I/O for performance
        std::ios::sync_with_stdio(false);
        std::cin.tie(nullptr);

        const auto t_total0 = Clock::now();

        // 2. Load Configuration
        // If an argument is provided, use it; otherwise, fall back to default
        std::string path = (argc > 1) ? argv[1] : "config/config.cfg";
        Config cfg(path);

        // 3. Initialize Data Readers (with path validation for security)
        CsvReader readerA(cfg.GetValidatedPath("Data.FutureA"));
        CsvReader readerB(cfg.GetValidatedPath("Data.FutureB"));
        StreamMerger merger(readerA, readerB);

        // 4. Initialize Core Components with Static Polymorphism
        // Create the concrete strategy directly on the stack for best locality
        Strategy strategy(cfg);
        PnlTracker pnl;

        // Pre-allocate log buffer to prevent heap fragmentation during hot loop
        std::string tradeBuf;
        tradeBuf.reserve(kTradeLogBufferSize);

        // Create simulation engine with strategy and PnL tracker
        SimulationEngine engine(std::move(strategy), pnl, tradeBuf);

        // 5. Simulation Loop Variables
        MarketEvent ev{};
        long long lastTime = 0;
        std::uint64_t events = 0;
        long long nextPrintTime = 0;

#ifdef ENABLE_PER_EVENT_TIMING
        double onEventMsSum = 0.0;
#endif

        const auto t_loop0 = Clock::now();

        // 6. Main Event Loop (Hot Path)
        while (merger.ReadNext(ev)) {
            lastTime = ev.sendingTime;

#ifdef ENABLE_PER_EVENT_TIMING
            const auto t0 = Clock::now();
#endif

            // Static dispatch happens here
            engine.OnEvent(ev);

#ifdef ENABLE_PER_EVENT_TIMING
            const auto t1 = Clock::now();
            onEventMsSum += Ms(t0, t1);
#endif

            // Periodic PnL Snapshot printing
            if (ev.sendingTime >= nextPrintTime) {
                if (nextPrintTime != 0) {
                    std::cout << ev.sendingTime << ",PNL," << engine.GetTotalPnl() << ","
                        << engine.GetLastMidB() << "," << engine.GetLastMidA()
                        << "\n";
                }
                nextPrintTime = ev.sendingTime + kPnlPrintIntervalNs;
            }

            ++events;

            if (engine.IsStopped()) {
                break;
            }
        }

        const auto t_loop1 = Clock::now();

        // 7. End of Day Cleanup
        engine.OnEndOfDay(lastTime);

        // 8. Output Results
        std::cout << tradeBuf; // Dump the pre-allocated trade log
        engine.PrintSummary(std::cout);

        const auto t_total1 = Clock::now();

        // 9. Performance Metrics
        const double loopMs = Ms(t_loop0, t_loop1);
        const double totalMs = Ms(t_total0, t_total1);
        const double loopSec = Sec(t_loop0, t_loop1);

        std::cout << "\nTiming Statistics\n";
        std::cout << "Events processed: " << events << "\n";
        std::cout << "Loop time: " << loopMs << " ms\n";
        std::cout << "Total time: " << totalMs << " ms\n";
        std::cout << "Throughput: " << (loopSec > 0.0 ? (events / loopSec) : 0.0) << " events/sec\n";
#ifdef ENABLE_PER_EVENT_TIMING
        std::cout << "Avg OnEvent: " << (events ? (onEventMsSum / events) : 0.0) << " ms\n";
#endif

    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}