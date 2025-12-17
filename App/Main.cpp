#include <iostream>
#include "CsvReader.h"
#include "StreamMerger.h"

int main() {
    ArbSim::CsvReader readerA("FutureA.csv");
    ArbSim::CsvReader readerB("FutureB.csv");

    ArbSim::StreamMerger merger(readerA, readerB);

    ArbSim::MarketEvent ev{};
    for (int i = 0; i < 10; i++) 
    {
        if (!merger.ReadNext(ev))
        {
            break;
        }

        std::cout
            << ev.sendingTime << ","
            << ArbSim::InstrumentToString(ev.instrumentId) << ","
            << ev.bid << "," << ev.ask
            << "\n";
    }

    return 0;
}
