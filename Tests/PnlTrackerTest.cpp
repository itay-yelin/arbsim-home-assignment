#include "core/PnlTracker.h"
#include "core/MarketData.h"
#include <gtest/gtest.h>


using namespace ArbSim;

class PnlTrackerTest : public ::testing::Test {
protected:
  PnlTracker pnl;

  // Helper to create a dummy quote
  MarketEvent CreateQuote(double bid, double ask) {
    MarketEvent ev{};
    ev.bid = bid;
    ev.ask = ask;
    // PnlTracker uses CalcMid = (bid+ask)*0.5
    return ev;
  }
};

// 1. Sanity check: Everything starts at zero
TEST_F(PnlTrackerTest, InitialState) {
  EXPECT_EQ(pnl.GetPositionB(), 0);
  EXPECT_DOUBLE_EQ(pnl.GetTotalPnl(), 0.0);
  EXPECT_EQ(pnl.GetTradedLots(), 0);
  EXPECT_FALSE(pnl.HasMidB());
}

// 2. Mark-to-Market Logic (Unrealized PnL)
TEST_F(PnlTrackerTest, MarkToMarket_LongPosition) {
  // Inject initial price: Bid=100, Ask=102 -> Mid=101
  pnl.OnQuoteB(CreateQuote(100.0, 102.0));

  // Buy 1 lot at 102.0 (Simulating taking the Ask)
  // Cash = -102, Pos = +1
  pnl.ApplyTradeB(1000, Side::Buy, 102.0, 1);

  // Price stays same: PnL = Cash(-102) + Pos(1)*Mid(101) = -1.0 (Spread cost)
  EXPECT_DOUBLE_EQ(pnl.GetTotalPnl(), -1.0);

  // Price moves UP: Bid=104, Ask=106 -> Mid=105
  pnl.OnQuoteB(CreateQuote(104.0, 106.0));

  // New PnL = -102 + 1*105 = +3.0
  EXPECT_DOUBLE_EQ(pnl.GetTotalPnl(), 3.0);
}

// 3. Realized PnL (Round Trip)
TEST_F(PnlTrackerTest, RealizedPnl_ShortRoundTrip) {
  // Sell 1 at 100.0
  pnl.ApplyTradeB(1000, Side::Sell, 100.0, 1);

  EXPECT_EQ(pnl.GetPositionB(), -1);
  EXPECT_DOUBLE_EQ(pnl.GetCash(), 100.0);

  // Update Mid to 90 (Profit scenario for short)
  pnl.OnQuoteB(CreateQuote(89.0, 91.0)); // Mid=90

  // Buy back at 91.0
  pnl.ApplyTradeB(1001, Side::Buy, 91.0, 1);

  // Position closed
  EXPECT_EQ(pnl.GetPositionB(), 0);

  // Cash = 100.0 - 91.0 = 9.0
  // PnL = Cash(9.0) + Pos(0)*Mid(90) = 9.0
  EXPECT_DOUBLE_EQ(pnl.GetTotalPnl(), 9.0);
}

// 4. Exposure Tracking
TEST_F(PnlTrackerTest, TracksMaxExposure) {
  // Buy 1
  pnl.ApplyTradeB(1, Side::Buy, 100, 1);
  EXPECT_EQ(pnl.GetMaxAbsExposure(), 1);

  // Buy 2 more (Total 3)
  pnl.ApplyTradeB(2, Side::Buy, 100, 2);
  EXPECT_EQ(pnl.GetMaxAbsExposure(), 3);

  // Sell 1 (Total 2) -> Max should still be 3
  pnl.ApplyTradeB(3, Side::Sell, 100, 1);
  EXPECT_EQ(pnl.GetMaxAbsExposure(), 3);
}

// 5. Flattening Logic
TEST_F(PnlTrackerTest, FlattenAtMid) {
  pnl.OnQuoteB(CreateQuote(100, 100));   // Mid 100
  pnl.ApplyTradeB(1, Side::Buy, 100, 5); // Long 5

  // Price moves to 110
  pnl.OnQuoteB(CreateQuote(110, 110));

  pnl.FlattenAtMid(2);

  EXPECT_EQ(pnl.GetPositionB(), 0);
  // Bought 5 @ 100 (-500), "Sold" 5 @ 110 (+550) -> PnL 50
  EXPECT_DOUBLE_EQ(pnl.GetTotalPnl(), 50.0);
}
