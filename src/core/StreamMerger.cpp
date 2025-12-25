#include "StreamMerger.h"

namespace ArbSim {

StreamMerger::StreamMerger(CsvReader &readerA, CsvReader &readerB,
                           unsigned int seed)
    : readerA_(readerA), readerB_(readerB), hasNextA_(false), hasNextB_(false),
      rng_(seed) // Initialize with fixed seed
      ,
      coinFlip_(0, 1) // Define range [0, 1]
{}

void StreamMerger::LoadNextAIfNeeded() {
  if (hasNextA_) {
    return;
  }

  MarketEvent ev{};
  if (readerA_.ReadNextEvent(ev)) {
    nextA_ = ev;
    hasNextA_ = true;
  } else {
    hasNextA_ = false;
  }
}

void StreamMerger::LoadNextBIfNeeded() {
  if (hasNextB_) {
    return;
  }

  MarketEvent ev{};
  if (readerB_.ReadNextEvent(ev)) {
    nextB_ = ev;
    hasNextB_ = true;
  } else {
    hasNextB_ = false;
  }
}

bool StreamMerger::ReadNext(MarketEvent &outEvent) {
  LoadNextAIfNeeded();
  LoadNextBIfNeeded();

  if (!hasNextA_ && !hasNextB_) {
    return false;
  }

  // Case: Only A has data
  if (hasNextA_ && !hasNextB_) {
    outEvent = nextA_;
    hasNextA_ = false;
    return true;
  }

  // Case: Only B has data
  if (!hasNextA_ && hasNextB_) {
    outEvent = nextB_;
    hasNextB_ = false;
    return true;
  }

  // Case: Both have data - Tie Breaking Logic
  bool pickA = false;

  if (nextA_.sendingTime < nextB_.sendingTime) {
    pickA = true;
  } else if (nextA_.sendingTime > nextB_.sendingTime) {
    pickA = false;
  } else {
    // Timestamps are equal!
    // Use the deterministic RNG to decide.
    // 1 = Pick A, 0 = Pick B
    pickA = (coinFlip_(rng_) == 1);
  }

  if (pickA) {
    outEvent = nextA_;
    hasNextA_ = false;
    return true;
  } else {
    outEvent = nextB_;
    hasNextB_ = false;
    return true;
  }
}

} // namespace ArbSim