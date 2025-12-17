#ifndef STREAM_MERGER_H
#define STREAM_MERGER_H

#include <optional>
#include "CsvReader.h"

namespace ArbSim {

    class StreamMerger {
    public:
        StreamMerger(CsvReader& readerA, CsvReader& readerB);

        // Returns false on EOF of both streams.
        bool ReadNext(MarketEvent& outEvent);

    private:
        CsvReader& readerA_;
        CsvReader& readerB_;

        std::optional<MarketEvent> nextA_;
        std::optional<MarketEvent> nextB_;

        void LoadNextAIfNeeded();
        void LoadNextBIfNeeded();
    };

} // namespace ArbSim

#endif // STREAM_MERGER_H
