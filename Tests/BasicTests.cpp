#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>
#include <cmath>
#include <sstream>
#include <fstream>
#include <cstdio>   // std::remove

#include <windows.h>

#include "CsvReader.h"
#include "StreamMerger.h"
#include "MarketData.h"
#include "PnlTracker.h"
#include "Strategy.h"
#include "SimulationEngine.h"

using namespace ArbSim;

//================= Helpers =================//

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
        throw std::runtime_error(message);
}

static void RequireNear(double actual, double expected, double eps, const std::string& message)
{
    if (std::abs(actual - expected) > eps)
        throw std::runtime_error(message + " actual=" + std::to_string(actual) + " expected=" + std::to_string(expected));
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

static MarketEvent MakeQuote(long long t, InstrumentId inst, double bid, double ask, int bidSize, int askSize)
{
    MarketEvent e{};
    e.sendingTime = t;
    e.instrumentId = inst;
    e.eventTypeId = 0;
    e.bidSize = bidSize;
    e.bid = bid;
    e.ask = ask;
    e.askSize = askSize;
    return e;
}

static MarketEvent MakeQuote(long long t, InstrumentId inst, double bid, double ask)
{
    return MakeQuote(t, inst, bid, ask, /*bidSize*/100, /*askSize*/100);
}

static void WriteTextFile(const std::string& path, const std::string& content)
{
    std::ofstream f(path, std::ios::binary);
    Require(f.good(), "Failed to create temp file: " + path);
    f << content;
    Require(f.good(), "Failed to write temp file: " + path);
}

//================= SimulationEngine tests =================//

void TestSimulationEngine_NoTradeUntilBothQuotes()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 1.0;
    p.MaxAbsExposureLots = 2;
    p.StopLossPnl = -50.0;

    std::ostringstream log;
    SimulationEngine eng(Strategy(p), PnlTracker(), log);

    eng.OnEvent(MakeQuote(100, InstrumentId::FutureA, 99.0, 101.0));
    Require(log.str().empty(), "SimEngine: traded before having both quotes");

    eng.OnEvent(MakeQuote(101, InstrumentId::FutureB, 99.0, 101.0));
    Require(CountSubstr(log.str(), ",BUY,") == 0 && CountSubstr(log.str(), ",SELL,") == 0,
        "SimEngine: unexpected trade when no executable edge exists");

    PrintOk("SimulationEngine no trade until both quotes");
}

void TestSimulationEngine_BuyBlocked_WhenAskSizeZero()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 1.0;
    p.MaxAbsExposureLots = 2;
    p.StopLossPnl = -50.0;

    std::ostringstream log;
    SimulationEngine eng(Strategy(p), PnlTracker(), log);

    // Force BuyB signal: A_bid - B_ask >= 1
    eng.OnEvent(MakeQuote(100, InstrumentId::FutureA, 101.0, 102.0, 100, 100));
    // B_ask=100, but askSize=0 => should not buy
    eng.OnEvent(MakeQuote(101, InstrumentId::FutureB, 99.0, 100.0, 100, 0));

    Require(CountSubstr(log.str(), ",BUY,FutureB,1,") == 0, "Expected no BUY when askSize is 0");
    PrintOk("SimulationEngine blocks BuyB when askSize=0");
}


void TestSimulationEngine_SellB_WhenExecutableSellEdge()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 1.0;
    p.MaxAbsExposureLots = 2;
    p.StopLossPnl = -50.0;

    std::ostringstream log;
    SimulationEngine eng(Strategy(p), PnlTracker(), log);

    // sellEdge = B_bid - A_ask
    // A_ask = 100, B_bid = 101 => sellEdge = 1 => SellB
    eng.OnEvent(MakeQuote(100, InstrumentId::FutureA, 99.0, 100.0));
    eng.OnEvent(MakeQuote(101, InstrumentId::FutureB, 101.0, 102.0));

    Require(CountSubstr(log.str(), ",SELL,FutureB,1,") == 1, "SimEngine: expected one SELL");
    PrintOk("SimulationEngine SellB on executable sellEdge");
}

void TestSimulationEngine_BuyB_WhenExecutableBuyEdge()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 1.0;
    p.MaxAbsExposureLots = 2;
    p.StopLossPnl = -50.0;

    std::ostringstream log;
    SimulationEngine eng(Strategy(p), PnlTracker(), log);

    // buyEdge = A_bid - B_ask
    // A_bid = 101, B_ask = 100 => buyEdge = 1 => BuyB
    eng.OnEvent(MakeQuote(100, InstrumentId::FutureA, 101.0, 102.0));
    eng.OnEvent(MakeQuote(101, InstrumentId::FutureB, 99.0, 100.0));

    Require(CountSubstr(log.str(), ",BUY,FutureB,1,") == 1, "SimEngine: expected one BUY");
    PrintOk("SimulationEngine BuyB on executable buyEdge");
}

void TestSimulationEngine_StopLoss_ClosesAsTradeAndStops()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 1.0;
    p.MaxAbsExposureLots = 10;
    p.StopLossPnl = -0.5;

    std::ostringstream log;
    SimulationEngine eng(Strategy(p), PnlTracker(), log);

    // Force BuyB:
    // A_bid - B_ask >= 1
    // A_bid = 101, B_ask = 100 => buyEdge=1 => BuyB at 100
    eng.OnEvent(MakeQuote(100, InstrumentId::FutureA, 101.0, 102.0));
    eng.OnEvent(MakeQuote(101, InstrumentId::FutureB, 99.0, 100.0));

    // Drop midB: bid=98 ask=99 => mid=98.5
    // PnL approx = cash(-100) + pos(1)*98.5 = -1.5 < -0.5 => stop and close at mid
    eng.OnEvent(MakeQuote(102, InstrumentId::FutureB, 98.0, 99.0));

    const std::string out = log.str();
    Require(CountSubstr(out, "STOP_LOSS_CLOSE") == 1, "SimEngine: expected STOP_LOSS_CLOSE tag");
    Require(CountSubstr(out, ",BUY,FutureB,") + CountSubstr(out, ",SELL,FutureB,") >= 2,
        "SimEngine: expected at least open trade and stop-loss close trade");

    const std::string before = log.str();

    // After stop, should not trade anymore
    eng.OnEvent(MakeQuote(103, InstrumentId::FutureA, 50.0, 51.0));
    eng.OnEvent(MakeQuote(104, InstrumentId::FutureB, 200.0, 201.0));

    Require(log.str() == before, "SimEngine: expected no further logs after stop");
    PrintOk("SimulationEngine stop loss closes as trade and stops");
}

void TestSimulationEngine_EndOfDayClose_Tagged()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 1.0;
    p.MaxAbsExposureLots = 10;
    p.StopLossPnl = -50.0;

    std::ostringstream log;
    SimulationEngine eng(Strategy(p), PnlTracker(), log);

    // Open position: BuyB
    eng.OnEvent(MakeQuote(100, InstrumentId::FutureA, 101.0, 102.0));
    eng.OnEvent(MakeQuote(101, InstrumentId::FutureB, 99.0, 100.0));

    eng.OnEndOfDay(200);

    Require(CountSubstr(log.str(), "EOD_CLOSE") == 1, "SimEngine: expected EOD_CLOSE tag");
    PrintOk("SimulationEngine end-of-day close tagged");
}

//================= Strategy tests (new Decide signature) =================//

void TestStrategyReturnsNone_WhenEdgesSmall()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 2.0;
    p.MaxAbsExposureLots = 2;
    p.StopLossPnl = -50.0;

    Strategy s(p);

    StrategyAction a = s.Decide(/*sellEdge*/1.999, /*buyEdge*/1.999, /*pos*/0, /*pnl*/0.0);
    Require(a == StrategyAction::None, "Strategy: expected None when edges are below threshold");
    PrintOk("Strategy returns None when edges small");
}

void TestStrategySells_WhenSellEdgeBig()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 2.0;
    p.MaxAbsExposureLots = 2;
    p.StopLossPnl = -50.0;

    Strategy s(p);

    StrategyAction a = s.Decide(/*sellEdge*/2.0, /*buyEdge*/0.0, /*pos*/0, /*pnl*/0.0);
    Require(a == StrategyAction::SellB, "Strategy: expected SellB when sellEdge big");
    PrintOk("Strategy sells when sellEdge big");
}

void TestStrategyBuys_WhenBuyEdgeBig()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 2.0;
    p.MaxAbsExposureLots = 2;
    p.StopLossPnl = -50.0;

    Strategy s(p);

    StrategyAction a = s.Decide(/*sellEdge*/0.0, /*buyEdge*/2.0, /*pos*/0, /*pnl*/0.0);
    Require(a == StrategyAction::BuyB, "Strategy: expected BuyB when buyEdge big");
    PrintOk("Strategy buys when buyEdge big");
}

void TestStrategyExposure_PostTrade_AllowsSellFromPlusY()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 1.0;
    p.MaxAbsExposureLots = 2;
    p.StopLossPnl = -50.0;

    Strategy s(p);

    // pos=+2, SellB would go to +1 which is allowed
    StrategyAction a = s.Decide(/*sellEdge*/1.0, /*buyEdge*/0.0, /*pos*/+2, /*pnl*/0.0);
    Require(a == StrategyAction::SellB, "Strategy: expected SellB allowed from +Y");
    PrintOk("Strategy allows SellB from +Y");
}

void TestStrategyExposure_PostTrade_BlocksBuyAtPlusY()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 1.0;
    p.MaxAbsExposureLots = 2;
    p.StopLossPnl = -50.0;

    Strategy s(p);

    // pos=+2, BuyB would go to +3 which is blocked
    StrategyAction a = s.Decide(/*sellEdge*/0.0, /*buyEdge*/1.0, /*pos*/+2, /*pnl*/0.0);
    Require(a == StrategyAction::None, "Strategy: expected BuyB blocked at +Y");
    PrintOk("Strategy blocks BuyB at +Y");
}

void TestStrategyStopsOnStopLoss()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 1.0;
    p.MaxAbsExposureLots = 2;
    p.StopLossPnl = -10.0;

    Strategy s(p);

    StrategyAction a = s.Decide(/*sellEdge*/100.0, /*buyEdge*/100.0, /*pos*/0, /*pnl*/-11.0);
    Require(a == StrategyAction::None, "Strategy: expected None when stop loss breached");
    PrintOk("Strategy returns None when stop loss breached");
}

void TestSimulationEngine_SellBlocked_WhenBidSizeZero()
{
    StrategyParams p{};
    p.MinArbitrageEdge = 1.0;
    p.MaxAbsExposureLots = 2;
    p.StopLossPnl = -50.0;

    std::ostringstream log;
    SimulationEngine eng(Strategy(p), PnlTracker(), log);

    // Force SellB signal: B_bid - A_ask >= 1
    eng.OnEvent(MakeQuote(100, InstrumentId::FutureA, 99.0, 100.0, 100, 100));
    // B_bid=101, but bidSize=0 => should not sell
    eng.OnEvent(MakeQuote(101, InstrumentId::FutureB, 101.0, 102.0, 0, 100));

    Require(CountSubstr(log.str(), ",SELL,FutureB,1,") == 0, "Expected no SELL when bidSize is 0");
    PrintOk("SimulationEngine blocks SellB when bidSize=0");
}

//================= CsvReader tests =================//

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
        count++;

    Require(count > 0, "CsvReader: expected at least one row");
    PrintOk("CsvReader EOF handling");
}

//================= StreamMerger tests =================//

void TestStreamMergerTieBreak_AFirstOnEqualTimestamp()
{
    const std::string fileA = "Data/_tmp_A_equal_ts.csv";
    const std::string fileB = "Data/_tmp_B_equal_ts.csv";

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

        MarketEvent e1{}, e2{}, e3{}, e4{};

        Require(merger.ReadNext(e1), "TieBreak: expected first event");
        Require(merger.ReadNext(e2), "TieBreak: expected second event");
        Require(merger.ReadNext(e3), "TieBreak: expected third event");
        Require(merger.ReadNext(e4), "TieBreak: expected fourth event");

        Require(e1.sendingTime == 1000, "TieBreak: e1 time mismatch");
        Require(e2.sendingTime == 1000, "TieBreak: e2 time mismatch");

        Require(e1.instrumentId == InstrumentId::FutureA, "TieBreak: expected FutureA first on equal timestamp");
        Require(e2.instrumentId == InstrumentId::FutureB, "TieBreak: expected FutureB second on equal timestamp");

        Require(e3.sendingTime == 1001 && e3.instrumentId == InstrumentId::FutureA, "TieBreak: expected A@1001 third");
        Require(e4.sendingTime == 1002 && e4.instrumentId == InstrumentId::FutureB, "TieBreak: expected B@1002 fourth");

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

    MarketEvent prev{}, curr{};
    bool hasPrev = false;

    for (int i = 0; i < 2000; i++)
    {
        if (!merger.ReadNext(curr))
            break;

        if (hasPrev)
            Require(curr.sendingTime >= prev.sendingTime, "StreamMerger: events not in chronological order");

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
            break;

        if (ev.instrumentId == InstrumentId::FutureB)
        {
            sawB = true;
            break;
        }
    }

    Require(sawB, "StreamMerger: expected to see FutureB event");
    PrintOk("StreamMerger contains FutureB events");
}

//================= PnlTracker tests =================//

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

//================= Test Runner =================//

int main()
{
    try
    {
        // CsvReader tests
        TestCsvReaderReadsFirstLine();
        TestCsvReaderEof();

        // StreamMerger tests
        TestStreamMergerOrdering();
        TestStreamMergerContainsFutureB();
        TestStreamMergerTieBreak_AFirstOnEqualTimestamp();

        // PnlTracker tests
        TestPnlTrackerInitialState();
        TestPnlTrackerBuyAndMarkToMarket();
        TestPnlTrackerRoundTrip();
        TestPnlTrackerMaxExposure();

        // Strategy tests
        TestStrategyReturnsNone_WhenEdgesSmall();
        TestStrategySells_WhenSellEdgeBig();
        TestStrategyBuys_WhenBuyEdgeBig();
        TestStrategyExposure_PostTrade_AllowsSellFromPlusY();
        TestStrategyExposure_PostTrade_BlocksBuyAtPlusY();
        TestStrategyStopsOnStopLoss();

        // SimulationEngine tests
        TestSimulationEngine_NoTradeUntilBothQuotes();
        TestSimulationEngine_SellB_WhenExecutableSellEdge();
        TestSimulationEngine_BuyB_WhenExecutableBuyEdge();
        TestSimulationEngine_StopLoss_ClosesAsTradeAndStops();
        TestSimulationEngine_EndOfDayClose_Tagged();
        TestSimulationEngine_BuyBlocked_WhenAskSizeZero();
        TestSimulationEngine_SellBlocked_WhenBidSizeZero();
    }
    catch (const std::exception& e)
    {
        PrintLine(std::string("[FAILED] ") + e.what());
        return 1;
    }

    PrintLine("All tests passed successfully");
    return 0;
}
