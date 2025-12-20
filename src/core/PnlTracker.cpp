#include "PnlTracker.h"

#include <cmath>
#include <limits>

namespace ArbSim
{

    PnlTracker::PnlTracker()
        : positionB_(0),
        cash_(0.0),
        hasMidB_(false),
        lastMidB_(0.0),
        totalPnl_(0.0),
        bestPnl_(-std::numeric_limits<double>::infinity()),
        worstPnl_(std::numeric_limits<double>::infinity()),
        maxAbsExposure_(0),
        tradedLots_(0)
    {
    }

    int PnlTracker::GetPositionB() const
    {
        return positionB_;
    }

    double PnlTracker::GetCash() const
    {
        return cash_;
    }

    bool PnlTracker::HasMidB() const
    {
        return hasMidB_;
    }

    double PnlTracker::GetLastMidB() const
    {
        return lastMidB_;
    }

    double PnlTracker::GetTotalPnl() const
    {
        return totalPnl_;
    }

    double PnlTracker::GetBestPnl() const
    {
        return bestPnl_;
    }

    double PnlTracker::GetWorstPnl() const
    {
        return worstPnl_;
    }

    int PnlTracker::GetMaxAbsExposure() const
    {
        return maxAbsExposure_;
    }

    int PnlTracker::GetTradedLots() const
    {
        return tradedLots_;
    }

    double PnlTracker::CalcMid(const MarketEvent& ev)
    {
        return (ev.bid + ev.ask) * 0.5;
    }

    void PnlTracker::OnQuoteB(const MarketEvent& bEvent)
    {
        lastMidB_ = CalcMid(bEvent);
        hasMidB_ = true;

        MarkToMarket();
    }

    void PnlTracker::ApplyTradeB(long long /*time*/, Side side, double price, int quantity)
    {
        if (quantity <= 0)
        {
            return;
        }

        if (side == Side::Buy)
        {
            positionB_ += quantity;
            cash_ -= price * quantity;
        }
        else
        {
            positionB_ -= quantity;
            cash_ += price * quantity;
        }

        tradedLots_ += quantity;

        const int absPos = std::abs(positionB_);
        if (absPos > maxAbsExposure_)
        {
            maxAbsExposure_ = absPos;
        }

        MarkToMarket();
    }

    double PnlTracker::MarkToMarket()
    {
        if (!hasMidB_)
        {
            return totalPnl_;
        }

        totalPnl_ = cash_ + positionB_ * lastMidB_;
        UpdateExtremes();
        return totalPnl_;
    }

    void PnlTracker::UpdateExtremes()
    {
        if (totalPnl_ > bestPnl_)
        {
            bestPnl_ = totalPnl_;
        }

        if (totalPnl_ < worstPnl_)
        {
            worstPnl_ = totalPnl_;
        }
    }

    void PnlTracker::FlattenAtMid(long long /*time*/)
    {
        if (!hasMidB_)
        {
            return;
        }

        if (positionB_ == 0)
        {
            return;
        }

        cash_ += positionB_ * lastMidB_;
        positionB_ = 0;

        MarkToMarket();
    }

} // namespace ArbSim
