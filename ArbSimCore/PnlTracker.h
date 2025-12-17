#ifndef PNL_TRACKER_H
#define PNL_TRACKER_H

#include <cstddef>
#include "MarketData.h"

namespace ArbSim {

    class PnlTracker {
    public:
        PnlTracker();

        int GetPositionB() const;
        double GetCash() const;

        double GetLastMidB() const;
        bool HasMidB() const;

        double GetTotalPnl() const;
        double GetBestPnl() const;
        double GetWorstPnl() const;

        int GetMaxAbsExposure() const;

        int GetTradedLots() const;

        void OnQuoteB(const MarketEvent& bEvent);

        void ApplyTradeB(long long time, Side side, double price, int quantity);

        double MarkToMarket();

        void FlattenAtMid(long long time);

    private:
        int positionB_;
        double cash_;

        bool hasMidB_;
        double lastMidB_;

        double totalPnl_;
        double bestPnl_;
        double worstPnl_;

        int maxAbsExposure_;

        int tradedLots_;

        void UpdateExtremes();
        static double CalcMid(const MarketEvent& ev);
    };

} // namespace ArbSim

#endif // PNL_TRACKER_H
