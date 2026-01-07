#include "PnlTracker.h"

#include <cassert>
#include <climits>
#include <cmath>
#include <cstdlib>

namespace ArbSim {

PnlTracker::PnlTracker()
    : positionB_(0), cashInt_(0), lastMidBInt_(0), totalPnlInt_(0),
      bestPnlInt_(0), worstPnlInt_(0), hasMidB_(false), hasExtremes_(false),
      maxAbsExposure_(0), tradedLots_(0) {}

// --- Helpers ---
double PnlTracker::ToDouble(int64_t val) const {
  return static_cast<double>(val) / static_cast<double>(Multiplier);
}

int64_t PnlTracker::ToInt(double val) const {
  // round to nearest integer unit
  return std::llround(val * static_cast<double>(Multiplier));
}

// --- Getters ---
int PnlTracker::GetPositionB() const { return positionB_; }

double PnlTracker::GetCash() const { return ToDouble(cashInt_); }

bool PnlTracker::HasMidB() const { return hasMidB_; }

double PnlTracker::GetLastMidB() const { return ToDouble(lastMidBInt_); }

double PnlTracker::GetTotalPnl() const { return ToDouble(totalPnlInt_); }

double PnlTracker::GetBestPnl() const {
  return hasExtremes_ ? ToDouble(bestPnlInt_) : 0.0;
}

double PnlTracker::GetWorstPnl() const {
  return hasExtremes_ ? ToDouble(worstPnlInt_) : 0.0;
}

int PnlTracker::GetMaxAbsExposure() const { return maxAbsExposure_; }

int PnlTracker::GetTradedLots() const { return tradedLots_; }

// --- Logic ---

void PnlTracker::OnQuoteB(const MarketEvent &bEvent) {
  // Calculate Mid in double, then convert to int
  const double mid = (bEvent.bid + bEvent.ask) * 0.5;
  lastMidBInt_ = ToInt(mid);
  hasMidB_ = true;

  MarkToMarket();
}

void PnlTracker::ApplyTradeB(long long /*time*/, Side side, double price,
                             int quantity) {
  if (quantity <= 0) {
    return;
  }

  const int64_t priceInt = ToInt(price);
  const int64_t costInt = priceInt * quantity;

  if (side == Side::Buy) {
    positionB_ += quantity;
    cashInt_ -= costInt;
  } else {
    positionB_ -= quantity;
    cashInt_ += costInt;
  }

  tradedLots_ += quantity;

  const int absPos = std::abs(positionB_);
  if (absPos > maxAbsExposure_) {
    maxAbsExposure_ = absPos;
  }

  MarkToMarket();
}

void PnlTracker::MarkToMarket() {
  if (!hasMidB_) {
    return;
  }

#ifdef _DEBUG
  // Debug-only overflow check for position * price multiplication
  if (positionB_ != 0 && lastMidBInt_ != 0) {
    assert(std::abs(lastMidBInt_) < INT64_MAX / std::abs(positionB_) &&
           "PnlTracker: potential integer overflow in position * price");
  }
#endif

  // PnL = Cash + (Position * CurrentMidPrice)
  // All done in integers -> No floating point drift
  totalPnlInt_ = cashInt_ + (static_cast<int64_t>(positionB_) * lastMidBInt_);

  UpdateExtremes();
}

void PnlTracker::UpdateExtremes() {
  if (!hasExtremes_) {
    bestPnlInt_ = totalPnlInt_;
    worstPnlInt_ = totalPnlInt_;
    hasExtremes_ = true;
    return;
  }
  if (totalPnlInt_ > bestPnlInt_) {
    bestPnlInt_ = totalPnlInt_;
  }
  if (totalPnlInt_ < worstPnlInt_) {
    worstPnlInt_ = totalPnlInt_;
  }
}

void PnlTracker::FlattenAtMid(long long /*time*/) {
  if (!hasMidB_) {
    return;
  }

  if (positionB_ == 0) {
    return;
  }

  // Mark position to market to realize remaining PnL
  cashInt_ += (static_cast<int64_t>(positionB_) * lastMidBInt_);
  positionB_ = 0;

  MarkToMarket();
}

} // namespace ArbSim