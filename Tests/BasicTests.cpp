#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>
#include <cmath>

#include <windows.h>

#include "CsvReader.h"
#include "StreamMerger.h"
#include "MarketData.h"
#include "PnlTracker.h"
#include "Strategy.h"
#include <sstream>
#include "SimulationEngine.h"
#include <fstream>
#include <cstdio>   // std::remove

using namespace ArbSim;


static MarketEvent MakeQuote(long long t, InstrumentId inst, double bid, double ask)
{
    MarketEvent e{};
    e.sendingTime = t;
    e.instrumentId = inst;
    e.eventTypeId = 0;
    e.bidSize = 100;
    e.bid = bid;
    e.ask = ask;
    e.askSize = 100;
    return e;
}

static int CountSubstr(const std::string& s, const std::string& sub)
{
    int count = 0;
    std::size_t pos = 0;
    while (true)
    {
        pos = s.find(sub, pos);
        if (pos == std::string::npos) break;
        ++count;
        pos += sub.size();
    }
    return count;
}

static void PrintLine(const std::string& s)
{
    std::cout << s << std::endl;
    OutputDebugStringA((s + "\n").c_str());
}

static void PrintOk(const std::string& s)
{
    PrintLine(std::string("[OK] ") + s);
}

static void Require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

static void RequireNear(double actual, double expected, double eps, const std::string& message)
{
    if (std::abs(actual - expected) > eps)
    {
        throw std::runtime_error(message + " actual=" + std::to_string(actual) + " expected=" + std::to_string(expected));
    }
}


static void WriteTextFile(const std::string& path, const std::string& content)
{
    std::ofstream f(path, std::ios::binary);
    Require(f.good(), "Failed to create temp file: " + path);
    f << content;
    Require(f.good(), "Failed to write temp file: " + path);
}
//================= Test Cases =================//
void TestSimulationEngine_NoTradeUntilBothQuotes()
{
    StrategyParams params;
    params.MinArbitrageEdge = 1.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -50.0;

    std::ostringstream log;
    SimulationEngine eng(Strategy(params), PnlTracker(), log);

    eng.OnEvent(MakeQuote(100, InstrumentId::FutureA, 99.0, 101.0));
    Require(log.str().empty(), "SimEngine: traded before having both quotes");

    eng.OnEvent(MakeQuote(101, InstrumentId::FutureB, 99.0, 101.0));
    Require(CountSubstr(log.str(), ",BUY,") == 0 && CountSubstr(log.str(), ",SELL,") == 0,
        "SimEngine: unexpected trade when edge is 0");

    PrintOk("SimulationEngine no trade until both quotes");
}

void TestSimulationEngine_SellB_WhenBExpensive()
{
    StrategyParams params;
    params.MinArbitrageEdge = 1.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -50.0;

    std::ostringstream log;
    SimulationEngine eng(Strategy(params), PnlTracker(), log);

    // midA = 100.0
    eng.OnEvent(MakeQuote(100, InstrumentId::FutureA, 99.0, 101.0));
    // midB = 101.0 -> edge = +1.0 -> SellB
    eng.OnEvent(MakeQuote(101, InstrumentId::FutureB, 100.0, 102.0));

    Require(CountSubstr(log.str(), ",SELL,FutureB,1,") == 1, "SimEngine: expected one SELL");
    PrintOk("SimulationEngine sells when B expensive");
}

void TestSimulationEngine_BuyB_WhenBCheap()
{
    StrategyParams params;
    params.MinArbitrageEdge = 1.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -50.0;

    std::ostringstream log;
    SimulationEngine eng(Strategy(params), PnlTracker(), log);

    // midA = 101.0
    eng.OnEvent(MakeQuote(100, InstrumentId::FutureA, 100.0, 102.0));
    // midB = 100.0 -> edge = -1.0 -> BuyB
    eng.OnEvent(MakeQuote(101, InstrumentId::FutureB, 99.0, 101.0));

    Require(CountSubstr(log.str(), ",BUY,FutureB,1,") == 1, "SimEngine: expected one BUY");
    PrintOk("SimulationEngine buys when B cheap");
}

void TestSimulationEngine_StopLossFlattensAndStops()
{
    StrategyParams params;
    params.MinArbitrageEdge = 1.0;
    params.MaxAbsExposureLots = 10;
    params.StopLossPnl = -0.5; // very tight

    std::ostringstream log;
    SimulationEngine eng(Strategy(params), PnlTracker(), log);

    // Get both quotes with edge negative -> BuyB at B ask=101.0, midB=100.0
    eng.OnEvent(MakeQuote(100, InstrumentId::FutureA, 100.0, 102.0)); // midA=101
    eng.OnEvent(MakeQuote(101, InstrumentId::FutureB, 99.0, 101.0));  // midB=100 => BUY at 101

    // Now crash midB to force pnl below -0.5
    eng.OnEvent(MakeQuote(102, InstrumentId::FutureB, 98.0, 99.0));   // midB=98.5 => pnl ~ -2.5 => FLATTEN

    Require(CountSubstr(log.str(), ",FLATTEN,FutureB,0,") == 1, "SimEngine: expected FLATTEN on stop loss");

    const std::string before = log.str();

    // Try to trigger more trading after stop
    eng.OnEvent(MakeQuote(103, InstrumentId::FutureA, 90.0, 92.0));
    eng.OnEvent(MakeQuote(104, InstrumentId::FutureB, 110.0, 112.0));

    Require(log.str() == before, "SimEngine: expected no further logs after stop trading");
    PrintOk("SimulationEngine stop loss flattens and stops");
}

void TestSimulationEngine_EndOfDayFlattensWhenOpen()
{
    StrategyParams params;
    params.MinArbitrageEdge = 1.0;
    params.MaxAbsExposureLots = 10;
    params.StopLossPnl = -50.0;

    std::ostringstream log;
    SimulationEngine eng(Strategy(params), PnlTracker(), log);

    // Create one BUY so position != 0
    eng.OnEvent(MakeQuote(100, InstrumentId::FutureA, 100.0, 102.0)); // midA=101
    eng.OnEvent(MakeQuote(101, InstrumentId::FutureB, 99.0, 101.0));  // midB=100 => BUY

    eng.OnEndOfDay(200);

    Require(CountSubstr(log.str(), ",FLATTEN_EOD,FutureB,0,") == 1, "SimEngine: expected FLATTEN_EOD");
    PrintOk("SimulationEngine end-of-day flatten");
}

void TestStrategyReturnsNoneWhenEdgeSmall()
{
    StrategyParams params;
    params.MinArbitrageEdge = 2.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -50.0;

    Strategy strategy(params);

    StrategyAction action = strategy.Decide(
        100.0,
        101.0,
        0,
        0.0
    );

    Require(action == StrategyAction::None, "Strategy: expected None when edge is small");
    PrintOk("Strategy returns None when edge is small");
}

void TestStrategySellsWhenBExpensive()
{
    StrategyParams params;
    params.MinArbitrageEdge = 2.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -50.0;

    Strategy strategy(params);

    StrategyAction action = strategy.Decide(
        100.0,
        103.0,
        0,
        0.0
    );

    Require(action == StrategyAction::SellB, "Strategy: expected SellB when B is expensive vs A");
    PrintOk("Strategy sells when B is expensive vs A");
}

void TestStrategyBuysWhenBCheap()
{
    StrategyParams params;
    params.MinArbitrageEdge = 2.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -50.0;

    Strategy strategy(params);

    StrategyAction action = strategy.Decide(
        100.0,
        97.0,
        0,
        0.0
    );

    Require(action == StrategyAction::BuyB, "Strategy: expected BuyB when B is cheap vs A");
    PrintOk("Strategy buys when B is cheap vs A");
}

void TestStrategyRespectsExposureLimit()
{
    StrategyParams params;
    params.MinArbitrageEdge = 1.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -50.0;

    Strategy strategy(params);

    StrategyAction action1 = strategy.Decide(
        100.0,
        102.0,
        2,
        0.0
    );

    Require(action1 == StrategyAction::None, "Strategy: expected None when exposure limit reached (long)");

    StrategyAction action2 = strategy.Decide(
        100.0,
        98.0,
        -2,
        0.0
    );

    Require(action2 == StrategyAction::None, "Strategy: expected None when exposure limit reached (short)");

    PrintOk("Strategy respects exposure limit");
}

void TestStrategyStopsOnStopLoss()
{
    StrategyParams params;
    params.MinArbitrageEdge = 1.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -10.0;

    Strategy strategy(params);

    StrategyAction action = strategy.Decide(
        100.0,
        120.0,
        0,
        -11.0
    );

    Require(action == StrategyAction::None, "Strategy: expected None when stop loss breached");
    PrintOk("Strategy returns None when stop loss breached");
}

void TestCsvReaderReadsFirstLine()
{
    CsvReader reader("Data/FutureA.csv");

    MarketEvent ev{};
    bool ok = reader.ReadNextEvent(ev);

    Require(ok, "CsvReader: expected to read first line");
    Require(ev.instrumentId == InstrumentId::FutureA, "CsvReader: expected instrumentId FutureA");
    Require(ev.bid > 0.0, "CsvReader: expected bid > 0");
    Require(ev.ask >= ev.bid, "CsvReader: expected ask >= bid");

    PrintOk("CsvReader reads first line");
}

void TestCsvReaderEof()
{
    CsvReader reader("Data/FutureA.csv");

    MarketEvent ev{};
    int count = 0;

    while (reader.ReadNextEvent(ev))
    {
        count++;
    }

    Require(count > 0, "CsvReader: expected at least one row");
    PrintOk("CsvReader EOF handling");
}

void TestStreamMergerTieBreak_AFirstOnEqualTimestamp()
{
    const std::string fileA = "Data/_tmp_A_equal_ts.csv";
    const std::string fileB = "Data/_tmp_B_equal_ts.csv";

    // Same sendingTime in both files: 1000
    // StreamMerger should output FutureA first when equal timestamp.
    const std::string a =
        "1000,FutureA,0,1,10,11,1\n"
        "1001,FutureA,0,1,10,11,1\n";

    const std::string b =
        "1000,FutureB,0,1,20,21,1\n"
        "1002,FutureB,0,1,20,21,1\n";

    WriteTextFile(fileA, a);
    WriteTextFile(fileB, b);

    try
    {
        CsvReader readerA(fileA);
        CsvReader readerB(fileB);
        StreamMerger merger(readerA, readerB);

        MarketEvent e1{};
        MarketEvent e2{};
        MarketEvent e3{};
        MarketEvent e4{};

        Require(merger.ReadNext(e1), "TieBreak: expected first event");
        Require(merger.ReadNext(e2), "TieBreak: expected second event");
        Require(merger.ReadNext(e3), "TieBreak: expected third event");
        Require(merger.ReadNext(e4), "TieBreak: expected fourth event");

        Require(e1.sendingTime == 1000, "TieBreak: e1 time mismatch");
        Require(e2.sendingTime == 1000, "TieBreak: e2 time mismatch");

        Require(e1.instrumentId == InstrumentId::FutureA,
            "TieBreak: expected FutureA first on equal timestamp");
        Require(e2.instrumentId == InstrumentId::FutureB,
            "TieBreak: expected FutureB second on equal timestamp");

        Require(e3.sendingTime == 1001 && e3.instrumentId == InstrumentId::FutureA,
            "TieBreak: expected A@1001 third");
        Require(e4.sendingTime == 1002 && e4.instrumentId == InstrumentId::FutureB,
            "TieBreak: expected B@1002 fourth");

        PrintOk("StreamMerger tie-break: A first on equal timestamp");
    }
    catch (...)
    {
        std::remove(fileA.c_str());
        std::remove(fileB.c_str());
        throw;
    }

    std::remove(fileA.c_str());
    std::remove(fileB.c_str());
}

void TestStreamMergerOrdering()
{
    CsvReader readerA("Data/FutureA.csv");
    CsvReader readerB("Data/FutureB.csv");

    StreamMerger merger(readerA, readerB);

    MarketEvent prev{};
    MarketEvent curr{};
    bool hasPrev = false;

    for (int i = 0; i < 2000; i++)
    {
        if (!merger.ReadNext(curr))
        {
            break;
        }

        if (hasPrev)
        {
            Require(curr.sendingTime >= prev.sendingTime, "StreamMerger: events not in chronological order");
        }

        prev = curr;
        hasPrev = true;
    }

    Require(hasPrev, "StreamMerger: expected to read at least one event");
    PrintOk("StreamMerger keeps chronological order");
}

void TestStreamMergerContainsFutureB()
{
    CsvReader readerA("Data/FutureA.csv");
    CsvReader readerB("Data/FutureB.csv");

    StreamMerger merger(readerA, readerB);

    MarketEvent ev{};
    bool sawB = false;

    for (int i = 0; i < 2000; i++)
    {
        if (!merger.ReadNext(ev))
        {
            break;
        }

        if (ev.instrumentId == InstrumentId::FutureB)
        {
            sawB = true;
            break;
        }
    }

    Require(sawB, "StreamMerger: expected to see FutureB event");
    PrintOk("StreamMerger contains FutureB events");
}

void TestPnlTrackerInitialState()
{
    PnlTracker pnl;

    Require(pnl.GetPositionB() == 0, "PnlTracker: expected initial position 0");
    RequireNear(pnl.GetTotalPnl(), 0.0, 1e-12, "PnlTracker: expected initial PnL 0");

    PrintOk("PnlTracker initial state");
}

void TestPnlTrackerBuyAndMarkToMarket()
{
    PnlTracker pnl;

    MarketEvent quote{};
    quote.bid = 100.0;
    quote.ask = 102.0;

    pnl.OnQuoteB(quote);
    pnl.ApplyTradeB(0, Side::Buy, 102.0, 1);

    const double expectedMid = 101.0;
    const double expectedPnl = expectedMid - 102.0;

    Require(pnl.GetPositionB() == 1, "PnlTracker: expected position 1 after buy");
    RequireNear(pnl.GetTotalPnl(), expectedPnl, 1e-9, "PnlTracker: unexpected PnL after buy and MTM");

    PrintOk("PnlTracker buy and MTM");
}

void TestPnlTrackerRoundTrip()
{
    PnlTracker pnl;

    MarketEvent quote{};
    quote.bid = 100.0;
    quote.ask = 102.0;

    pnl.OnQuoteB(quote);

    pnl.ApplyTradeB(0, Side::Buy, 102.0, 1);
    pnl.ApplyTradeB(1, Side::Sell, 100.0, 1);

    Require(pnl.GetPositionB() == 0, "PnlTracker: expected position 0 after round trip");
    RequireNear(pnl.GetTotalPnl(), -2.0, 1e-9, "PnlTracker: unexpected realized PnL after round trip");

    PrintOk("PnlTracker round trip realized PnL");
}

void TestPnlTrackerMaxExposure()
{
    PnlTracker pnl;

    MarketEvent quote{};
    quote.bid = 10.0;
    quote.ask = 11.0;

    pnl.OnQuoteB(quote);

    pnl.ApplyTradeB(0, Side::Buy, 11.0, 2);
    pnl.ApplyTradeB(1, Side::Sell, 10.0, 1);

    Require(pnl.GetMaxAbsExposure() == 2, "PnlTracker: expected max abs exposure 2");
    PrintOk("PnlTracker max exposure");
}

int main()
{
    try
    {
        TestCsvReaderReadsFirstLine();
        TestCsvReaderEof();
        TestStreamMergerOrdering();
        TestStreamMergerContainsFutureB();
        TestStreamMergerTieBreak_AFirstOnEqualTimestamp();


		// PnlTracker tests
        TestPnlTrackerInitialState();
        TestPnlTrackerBuyAndMarkToMarket();
        TestPnlTrackerRoundTrip();
        TestPnlTrackerMaxExposure();

		// Strategy tests
        TestStrategyReturnsNoneWhenEdgeSmall();
        TestStrategySellsWhenBExpensive();
        TestStrategyBuysWhenBCheap();
        TestStrategyRespectsExposureLimit();
        TestStrategyStopsOnStopLoss();

		// SimulationEngine tests
        TestSimulationEngine_NoTradeUntilBothQuotes();
        TestSimulationEngine_SellB_WhenBExpensive();
        TestSimulationEngine_BuyB_WhenBCheap();
        TestSimulationEngine_StopLossFlattensAndStops();
        TestSimulationEngine_EndOfDayFlattensWhenOpen();


    }
    catch (const std::exception& e)
    {
        PrintLine(std::string("[FAILED] ") + e.what());
        return 1;
    }

    PrintLine("All tests passed successfully");
    return 0;
}
