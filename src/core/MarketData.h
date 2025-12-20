#ifndef MARKET_DATA_H
#define MARKET_DATA_H

#include <string>

namespace ArbSim
{

enum class InstrumentId
{
    FutureA,
    FutureB,
    Unknown
};

enum class Side
{
    Buy,
    Sell
};

struct MarketEvent
{
    long long sendingTime;
    InstrumentId instrumentId;
    int eventTypeId;
    int bidSize;
    double bid;
    double ask;
    int askSize;
};

inline std::string InstrumentToString(InstrumentId id)
{
    switch (id)
    {
        case InstrumentId::FutureA:
        {
            return "FutureA";
        }
        case InstrumentId::FutureB:
        {
            return "FutureB";
        }
        default:
        {
            return "Unknown";
        }
    }
}

inline InstrumentId StringToInstrument(const std::string& s)
{
    if (s == "FutureA")
    {
        return InstrumentId::FutureA;
    }

    if (s == "FutureB")
    {
        return InstrumentId::FutureB;
    }

    return InstrumentId::Unknown;
}

} // namespace ArbSim

#endif // MARKET_DATA_H
