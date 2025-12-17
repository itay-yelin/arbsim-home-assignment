#include <cassert>
#include <iostream>
#include <exception>

#include "CsvReader.h"
#include "StreamMerger.h"
#include "MarketData.h"
#include <windows.h>
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
    PrintLine("[OK] " + s);
}


void TestStrategyReturnsNoneWhenEdgeSmall()
{
    ArbSim::StrategyParams params;
    params.MinArbitrageEdge = 2.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -50.0;

    ArbSim::Strategy strategy(params);

    ArbSim::StrategyAction action = strategy.Decide(
        100.0,
        101.0,
        0,
        0.0
    );

    assert(action == ArbSim::StrategyAction::None);

    PrintOk("Strategy returns None when edge is small");
}

void TestStrategySellsWhenBExpensive()
{
    ArbSim::StrategyParams params;
    params.MinArbitrageEdge = 2.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -50.0;

    ArbSim::Strategy strategy(params);

    ArbSim::StrategyAction action = strategy.Decide(
        100.0,
        103.0,
        0,
        0.0
    );

    assert(action == ArbSim::StrategyAction::SellB);

    PrintOk("Strategy sells when B is expensive vs A");
}

void TestStrategyBuysWhenBCheap()
{
    ArbSim::StrategyParams params;
    params.MinArbitrageEdge = 2.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -50.0;

    ArbSim::Strategy strategy(params);

    ArbSim::StrategyAction action = strategy.Decide(
        100.0,
        97.0,
        0,
        0.0
    );

    assert(action == ArbSim::StrategyAction::BuyB);

    PrintOk("Strategy buys when B is cheap vs A");
}

void TestStrategyRespectsExposureLimit()
{
    ArbSim::StrategyParams params;
    params.MinArbitrageEdge = 1.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -50.0;

    ArbSim::Strategy strategy(params);

    ArbSim::StrategyAction action1 = strategy.Decide(
        100.0,
        102.0,
        2,
        0.0
    );

    assert(action1 == ArbSim::StrategyAction::None);

    ArbSim::StrategyAction action2 = strategy.Decide(
        100.0,
        98.0,
        -2,
        0.0
    );

    assert(action2 == ArbSim::StrategyAction::None);

    PrintOk("Strategy respects exposure limit");
}

void TestStrategyStopsOnStopLoss()
{
    ArbSim::StrategyParams params;
    params.MinArbitrageEdge = 1.0;
    params.MaxAbsExposureLots = 2;
    params.StopLossPnl = -10.0;

    ArbSim::Strategy strategy(params);

    ArbSim::StrategyAction action = strategy.Decide(
        100.0,
        120.0,
        0,
        -11.0
    );

    assert(action == ArbSim::StrategyAction::None);

    PrintOk("Strategy returns None when stop loss breached");
}


void TestCsvReaderReadsFirstLine()
{
    CsvReader reader("Data/FutureA.csv");

    MarketEvent ev{};
    bool ok = reader.ReadNextEvent(ev);

    assert(ok);
    assert(ev.instrumentId == InstrumentId::FutureA);
    assert(ev.bid > 0.0);
    assert(ev.ask >= ev.bid);

    PrintLine("CsvReader reads first line");
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

    assert(count > 0);

    PrintLine("CsvReader EOF handling");
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
            assert(curr.sendingTime >= prev.sendingTime);
        }

        prev = curr;
        hasPrev = true;
    }

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

    assert(sawB);

    PrintLine("StreamMerger contains FutureB events");
}


void TestPnlTrackerInitialState()
{
    ArbSim::PnlTracker pnl;

    assert(pnl.GetPositionB() == 0);
    assert(pnl.GetTotalPnl() == 0.0);

    PrintOk("PnlTracker initial state");
}

void TestPnlTrackerBuyAndMarkToMarket()
{
    ArbSim::PnlTracker pnl;

    ArbSim::MarketEvent quote{};
    quote.bid = 100.0;
    quote.ask = 102.0;

    pnl.OnQuoteB(quote);

    pnl.ApplyTradeB(0, ArbSim::Side::Buy, 102.0, 1);

    double expectedMid = 101.0;
    double expectedPnl = expectedMid - 102.0;

    assert(std::abs(pnl.GetTotalPnl() - expectedPnl) < 1e-9);
    assert(pnl.GetPositionB() == 1);

    PrintOk("PnlTracker buy and MTM");
}

void TestPnlTrackerRoundTrip()
{
    ArbSim::PnlTracker pnl;

    ArbSim::MarketEvent quote{};
    quote.bid = 100.0;
    quote.ask = 102.0;

    pnl.OnQuoteB(quote);

    pnl.ApplyTradeB(0, ArbSim::Side::Buy, 102.0, 1);
    pnl.ApplyTradeB(1, ArbSim::Side::Sell, 100.0, 1);

    assert(pnl.GetPositionB() == 0);
    assert(std::abs(pnl.GetTotalPnl() + 2.0) < 1e-9);

    PrintOk("PnlTracker round trip realized PnL");
}

void TestPnlTrackerMaxExposure()
{
    ArbSim::PnlTracker pnl;

    ArbSim::MarketEvent quote{};
    quote.bid = 10.0;
    quote.ask = 11.0;

    pnl.OnQuoteB(quote);

    pnl.ApplyTradeB(0, ArbSim::Side::Buy, 11.0, 2);
    pnl.ApplyTradeB(1, ArbSim::Side::Sell, 10.0, 1);

    assert(pnl.GetMaxAbsExposure() == 2);

    PrintOk("PnlTracker max exposure");
}

int main()
{
    try
    {
		// CsvReader tests, streamermer tests
        TestCsvReaderReadsFirstLine();
        TestCsvReaderEof();
        TestStreamMergerOrdering();
        TestStreamMergerContainsFutureB();

		// PnL Tracker tests
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
    }
    catch (const std::exception& e)
    {
        PrintLine(std::string("[FAILED] Exception: ") + e.what());
        return 1;
    }

    PrintLine("All tests passed successfully");
    return 0;
}