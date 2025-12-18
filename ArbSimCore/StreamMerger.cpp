#include "StreamMerger.h"

namespace ArbSim 
{

    StreamMerger::StreamMerger(CsvReader& readerA, CsvReader& readerB)
		: readerA_(readerA), readerB_(readerB), hasNextA_(false), hasNextB_(false)
    {    }

    void StreamMerger::LoadNextAIfNeeded() 
    {
        if (hasNextA_)
        {
            return;
        }

        MarketEvent ev{};
        if (readerA_.ReadNextEvent(ev))
        {
            nextA_ = ev;
            hasNextA_ = true;
        }
        else
        {
            hasNextA_ = false;
        }
    }

    void StreamMerger::LoadNextBIfNeeded() 
    {
        if (hasNextB_)
        {
            return;
        }

        MarketEvent ev{};
        if (readerB_.ReadNextEvent(ev))
        {
            nextB_ = ev;
            hasNextB_ = true;
        }
        else
        {
            hasNextB_ = false;
        }
    }

    bool StreamMerger::ReadNext(MarketEvent& outEvent) 
    {
        LoadNextAIfNeeded();
        LoadNextBIfNeeded();

        if (!hasNextA_ && !hasNextB_) 
        {
            return false;
        }

        if (hasNextA_ && hasNextB_) 
        {
            // tie-break: A first when equal timestamp
            if (nextA_.sendingTime <= nextB_.sendingTime)
            {
                outEvent = nextA_;
                hasNextA_ = false;
                return true;
            }
            else
            {
                outEvent = nextB_;
                hasNextB_ = false;
                return true;
            }
        }

        if (hasNextA_)
        {
            outEvent = nextA_;
            hasNextA_ = false;
            return true;
        }

        outEvent = nextB_;
        hasNextB_ = false;
        return true;
    }

} // namespace ArbSim
