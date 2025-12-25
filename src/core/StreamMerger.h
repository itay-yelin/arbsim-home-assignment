#ifndef STREAM_MERGER_H
#define STREAM_MERGER_H

#include "CsvReader.h"
#include "MarketData.h"
#include <random> // Required for random engine

namespace ArbSim {

class StreamMerger {
public:  
  StreamMerger(CsvReader &readerA, CsvReader &readerB, unsigned int seed = 42);

  bool ReadNext(MarketEvent &outEvent);

private:
  CsvReader &readerA_;
  CsvReader &readerB_;

  MarketEvent nextA_;
  MarketEvent nextB_;

  bool hasNextA_;
  bool hasNextB_;

  // Deterministic Random Number Generator
  std::mt19937 rng_;
  std::uniform_int_distribution<int> coinFlip_;

  void LoadNextAIfNeeded();
  void LoadNextBIfNeeded();
};

} // namespace ArbSim

#endif // STREAM_MERGER_H