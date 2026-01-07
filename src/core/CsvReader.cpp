#include "CsvReader.h"

#include <cstdlib> // Required for std::strtoll, std::strtod
#include <cstring> // Required for std::strncmp

namespace ArbSim {

CsvReader::CsvReader(const std::string &filePath)
    : filePath_(filePath), file_(filePath) {
  if (!file_.is_open()) {
    throw std::runtime_error("CsvReader: Failed to open file: " + filePath_);
  }
  // Reserve memory for typical line length to prevent early reallocations
  lineBuffer_.reserve(128);
}

bool CsvReader::IsOpen() const { return file_.is_open(); }

const std::string &CsvReader::GetFilePath() const { return filePath_; }

bool CsvReader::ReadNextNonEmptyLine() {
  while (std::getline(file_, lineBuffer_)) {
    if (!lineBuffer_.empty()) {
      return true;
    }
  }
  return false;
}

void CsvReader::StripTrailingCarriageReturn(std::string &line) {
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  }
}

bool CsvReader::ReadNextEvent(MarketEvent &event) {
  if (!ReadNextNonEmptyLine()) {
    return false;
  }

  StripTrailingCarriageReturn(lineBuffer_);

  // Validate field count (expect exactly 7 fields: 6 commas)
  int commaCount = 0;
  for (char c : lineBuffer_) {
    if (c == ',') ++commaCount;
  }
  if (commaCount != 6) {
    throw std::runtime_error("CSV format error: expected 7 fields (6 commas), got " +
                             std::to_string(commaCount + 1) + " fields in: " +
                             lineBuffer_.substr(0, 50) + (lineBuffer_.size() > 50 ? "..." : ""));
  }

  const char *ptr = lineBuffer_.data();
  const char *const end = ptr + lineBuffer_.size();

  auto skipComma = [&]() {
    if (ptr < end && *ptr == ',') {
      ++ptr;
      return true;
    }
    return false;
  };

  // r integer parsing 
  auto parseInt = [&](auto &out) -> bool {
    char *endPtr = nullptr;
    long long val = std::strtoll(ptr, &endPtr, 10);

    if (ptr == endPtr) {
      return false; // No digits found
    }

    out = static_cast<std::decay_t<decltype(out)>>(val);
    ptr = endPtr;
    return true;
  };

  // double parsing 
  auto parseDouble = [&](double &out) -> bool {
    char *endPtr = nullptr;
    out = std::strtod(ptr, &endPtr);

    if (ptr == endPtr) {
      return false; // No digits found
    }

    ptr = endPtr;
    return true;
  };

  // parse timestamp
  if (!parseInt(event.sendingTime)) {
    throw std::runtime_error("Parse error: sendingTime in " + lineBuffer_);
  }
  skipComma();

  // parse instrumentId 
  const char *tokenEnd = ptr;
  while (tokenEnd < end && *tokenEnd != ',') {
    ++tokenEnd;
  }

  // Calculate length 
  std::ptrdiff_t len = tokenEnd - ptr;

  // "FutureA" (length 7)
  if (len == 7 && std::strncmp(ptr, "FutureA", 7) == 0) {
    event.instrumentId = InstrumentId::FutureA;
  }
  // "FutureB" (length 7)
  else if (len == 7 && std::strncmp(ptr, "FutureB", 7) == 0) {
    event.instrumentId = InstrumentId::FutureB;
  } else {
    event.instrumentId = InstrumentId::Unknown;
  }

  ptr = tokenEnd;
  skipComma();

  // eventTypeId 
  if (!parseInt(event.eventTypeId)) {
    throw std::runtime_error("Parse error: eventTypeId in " + lineBuffer_);
  }
  skipComma();

  // bidSize
  if (!parseInt(event.bidSize)) {
    throw std::runtime_error("Parse error: bidSize in " + lineBuffer_);
  }
  skipComma();

  // bid 
  if (!parseDouble(event.bid)) {
    throw std::runtime_error("Parse error: bid in " + lineBuffer_);
  }
  skipComma();

  // ask 
  if (!parseDouble(event.ask)) {
    throw std::runtime_error("Parse error: ask in " + lineBuffer_);
  }
  skipComma();

  // askSize
  if (!parseInt(event.askSize)) {
    throw std::runtime_error("Parse error: askSize in " + lineBuffer_);
  }

  return true;
}

} // namespace ArbSim