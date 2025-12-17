#include <cassert>
#include <iostream>
#include <exception>

#include "CsvReader.h"
#include "StreamMerger.h"
#include "MarketData.h"
#include <windows.h>

using namespace ArbSim;

static void PrintLine(const std::string& s)
{
    std::cout << s << std::endl;
    OutputDebugStringA((s + "\n").c_str());
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

    PrintLine("StreamMerger keeps chronological order");
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


int main()
{
    try
    {
        TestCsvReaderReadsFirstLine();
        TestCsvReaderEof();
        TestStreamMergerOrdering();
        TestStreamMergerContainsFutureB();
    }
    catch (const std::exception& e)
    {
        PrintLine(std::string("[FAILED] Exception: ") + e.what());
        return 1;
    }

    PrintLine("All tests passed successfully");
    return 0;
}