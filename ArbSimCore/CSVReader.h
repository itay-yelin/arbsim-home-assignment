#ifndef CSV_READER_H
#define CSV_READER_H

#include <string>
#include <fstream>
#include <vector>
#include "MarketData.h"

namespace ArbSim {

    class CsvReader {
    public:
        explicit CsvReader(const std::string& filePath);

        bool IsOpen() const;
        const std::string& GetFilePath() const;

        bool ReadNextEvent(MarketEvent& event);

    private:
        std::string filePath_;
        std::ifstream file_;

        bool ReadNextNonEmptyLine(std::string& line);

        static void StripTrailingCarriageReturn(std::string& line);
        static std::vector<std::string> SplitCsvLine(const std::string& line);

        static long long ParseInt64(const std::string& token, const std::string& fieldName);
        static int ParseInt(const std::string& token, const std::string& fieldName);
        static double ParseDouble(const std::string& token, const std::string& fieldName);
    };

} // namespace ArbSim

#endif
