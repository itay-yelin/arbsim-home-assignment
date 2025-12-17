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

using namespace ArbSim;

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

        TestPnlTrackerInitialState();
        TestPnlTrackerBuyAndMarkToMarket();
        TestPnlTrackerRoundTrip();
        TestPnlTrackerMaxExposure();

        TestStrategyReturnsNoneWhenEdgeSmall();
        TestStrategySellsWhenBExpensive();
        TestStrategyBuysWhenBCheap();
        TestStrategyRespectsExposureLimit();
        TestStrategyStopsOnStopLoss();

    }
    catch (const std::exception& e)
    {
        PrintLine(std::string("[FAILED] ") + e.what());
        return 1;
    }

    PrintLine("All tests passed successfully");
    return 0;
}
