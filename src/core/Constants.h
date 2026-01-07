#ifndef ARBSIM_CONSTANTS_H
#define ARBSIM_CONSTANTS_H

#include <cstddef>
#include <cstdint>

namespace ArbSim {

// Buffer sizes
constexpr size_t kTradeLogBufferSize = 1 << 20;  // 1MB

// Time constants (nanoseconds)
constexpr int64_t kNanosecondsPerSecond = 1'000'000'000LL;
constexpr int64_t kPnlPrintIntervalNs = 60 * kNanosecondsPerSecond;  // 60 seconds

// Precision constants
constexpr int64_t kPnlMultiplier = 1'000'000;  // 6 decimal places
constexpr double kFloatCompareEpsilon = 1e-9;

} // namespace ArbSim

#endif // ARBSIM_CONSTANTS_H
