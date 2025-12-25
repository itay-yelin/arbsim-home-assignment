#ifndef CSV_READER_H
#define CSV_READER_H

#include "MarketData.h"
#include <charconv> // Required for std::from_chars
#include <fstream>
#include <string>
#include <vector>


namespace ArbSim {

class CsvReader {
public:
  explicit CsvReader(const std::string &filePath);

  bool IsOpen() const;
  const std::string &GetFilePath() const;

  bool ReadNextEvent(MarketEvent &event);

private:
  std::string filePath_;
  std::ifstream file_;

  // Reused buffer to minimize heap allocations
  std::string lineBuffer_;

  bool ReadNextNonEmptyLine();

  // Helper to strip CR if present
  static void StripTrailingCarriageReturn(std::string &line);
};

} // namespace ArbSim

#endif