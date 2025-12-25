#ifndef PNL_TRACKER_H
#define PNL_TRACKER_H

#include "MarketData.h"
#include <algorithm>
#include <cstdint> // For int64_t

namespace ArbSim
{

    class PnlTracker
    {
    public:
        PnlTracker();

        // Getters convert internal integer representation back to double for display
        int GetPositionB() const;
        double GetCash() const;
        bool HasMidB() const;
        double GetLastMidB() const;
        double GetTotalPnl() const;
        double GetBestPnl() const;
        double GetWorstPnl() const;
        int GetMaxAbsExposure() const;
        int GetTradedLots() const;

        void OnQuoteB(const MarketEvent& bEvent);
        void ApplyTradeB(long long time, Side side, double price, int quantity);
        
        // Helper to force a flatten (used by Stop Loss)
        void FlattenAtMid(long long time);

    private:
        // Precision Multiplier (e.g., 1,000,000 for 6 decimal places)
        static constexpr int64_t Multiplier = 1000000;

        int positionB_;
        
        // Internal state is now purely integer-based to prevent drift
        int64_t cashInt_; 
        int64_t lastMidBInt_;
        
        int64_t totalPnlInt_;
        int64_t bestPnlInt_;
        int64_t worstPnlInt_;

        bool hasMidB_;
        int maxAbsExposure_;
        int tradedLots_;

        // Helper to convert internal int64_t back to double
        double ToDouble(int64_t val) const;
        
        // Helper to convert external double to internal int64_t
        int64_t ToInt(double val) const;

        void MarkToMarket();
        void UpdateExtremes();
    };

} // namespace ArbSim

#endif // PNL_TRACKER_H