#include "StreamMerger.h"

namespace ArbSim 
{

    StreamMerger::StreamMerger(CsvReader& readerA, CsvReader& readerB)
        : readerA_(readerA), readerB_(readerB) 
    {    }

    void StreamMerger::LoadNextAIfNeeded() 
    {
        if (nextA_.has_value()) 
        {
            return;
        }

        MarketEvent ev{};
        if (readerA_.ReadNextEvent(ev)) 
        {
            nextA_ = ev;
        }
        else 
        {
            nextA_.reset();
        }
    }

    void StreamMerger::LoadNextBIfNeeded() 
    {
        if (nextB_.has_value()) {
            return;
        }

        MarketEvent ev{};
        if (readerB_.ReadNextEvent(ev)) 
        {
            nextB_ = ev;
        }
        else {
            nextB_.reset();
        }
    }

    bool StreamMerger::ReadNext(MarketEvent& outEvent) 
    {
        LoadNextAIfNeeded();
        LoadNextBIfNeeded();

        if (!nextA_.has_value() && !nextB_.has_value()) 
        {
            return false;
        }

        if (nextA_.has_value() && nextB_.has_value()) 
        {
            // tie-break: A first when equal timestamp
            if (nextA_->sendingTime <= nextB_->sendingTime) 
            {
                outEvent = *nextA_;
                nextA_.reset();
                return true;
            }
            else 
            {
                outEvent = *nextB_;
                nextB_.reset();
                return true;
            }
        }

        if (nextA_.has_value()) 
        {
            outEvent = *nextA_;
            nextA_.reset();
            return true;
        }

        outEvent = *nextB_;
        nextB_.reset();
        return true;
    }

} // namespace ArbSim
