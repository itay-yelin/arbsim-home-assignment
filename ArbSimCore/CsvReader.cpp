#include "CsvReader.h"

#include <sstream>
#include <stdexcept>
#include <vector>

namespace ArbSim
{

    CsvReader::CsvReader(const std::string& filePath)
        : filePath_(filePath), file_(filePath)
    {
        if (!file_.is_open())
        {
            throw std::runtime_error("CsvReader: Failed to open file: " + filePath_);
        }
    }

    bool CsvReader::IsOpen() const
    {
        return file_.is_open();
    }

    const std::string& CsvReader::GetFilePath() const
    {
        return filePath_;
    }

    bool CsvReader::ReadNextNonEmptyLine(std::string& line)
    {
        while (std::getline(file_, line))
        {
            if (!line.empty())
            {
                return true;
            }
        }
        return false;
    }

    void CsvReader::StripTrailingCarriageReturn(std::string& line)
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
    }

    bool CsvReader::ReadNextEvent(MarketEvent& event)
    {
        std::string line;
        if (!ReadNextNonEmptyLine(line))
        {
            return false;
        }

        StripTrailingCarriageReturn(line);

        std::vector<std::string> parts;
        parts.reserve(CsvColumn::ColumnCount);

        {
            std::stringstream ss(line);
            std::string token;
            while (std::getline(ss, token, ','))
            {
                parts.push_back(token);
            }
        }

        if (parts.size() != CsvColumn::ColumnCount)
        {
            throw std::runtime_error("CsvReader: Malformed CSV line (expected 7 columns): " + line);
        }

        try
        {
            std::size_t idx = 0;

            idx = 0;
            event.sendingTime = std::stoll(parts[CsvColumn::ColSendingTime], &idx);
            if (idx != parts[CsvColumn::ColSendingTime].size())
            {
                throw std::runtime_error("sendingTime trailing chars");
            }

            event.instrumentId = StringToInstrument(parts[CsvColumn::ColInstrumentId]);
            if (event.instrumentId == InstrumentId::Unknown)
            {
                throw std::runtime_error("unknown instrumentId: " + parts[CsvColumn::ColInstrumentId]);
            }

            idx = 0;
            event.eventTypeId = std::stoi(parts[CsvColumn::ColEventTypeId], &idx);
            if (idx != parts[CsvColumn::ColEventTypeId].size())
            {
                throw std::runtime_error("eventTypeId trailing chars");
            }

            idx = 0;
            event.bidSize = std::stoi(parts[CsvColumn::ColBidSize], &idx);
            if (idx != parts[CsvColumn::ColBidSize].size())
            {
                throw std::runtime_error("bidSize trailing chars");
            }

            idx = 0;
            event.bid = std::stod(parts[CsvColumn::ColBid], &idx);
            if (idx != parts[CsvColumn::ColBid].size())
            {
                throw std::runtime_error("bid trailing chars");
            }

            idx = 0;
            event.ask = std::stod(parts[CsvColumn::ColAsk], &idx);
            if (idx != parts[CsvColumn::ColAsk].size())
            {
                throw std::runtime_error("ask trailing chars");
            }

            idx = 0;
            event.askSize = std::stoi(parts[CsvColumn::ColAskSize], &idx);
            if (idx != parts[CsvColumn::ColAskSize].size())
            {
                throw std::runtime_error("askSize trailing chars");
            }
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(std::string("CsvReader: Parse error: ") + e.what() + " | line: " + line);
        }

        return true;
    }

} // namespace ArbSim
