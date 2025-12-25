#include "Config.h"

#include <fstream>
#include <stdexcept>


namespace {

// Helper to remove leading/trailing whitespace
std::string Trim(const std::string &str) {
  const auto start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }

  const auto end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

} // namespace

namespace ArbSim {

Config::Config(const std::string &path) {
  std::ifstream f(path);
  if (!f) {
    throw std::runtime_error("Config: Failed to open file: " + path);
  }

  std::string line;
  while (std::getline(f, line)) {

    // Strip comments (everything after #)
    const auto commentPos = line.find('#');
    if (commentPos != std::string::npos) {
      line = line.substr(0, commentPos);
    }

    // Trim whitespace around the whole line
    line = Trim(line);

    if (line.empty()) {
      continue;
    }

    // Parse Key=Value
    const auto sepPos = line.find('=');
    if (sepPos == std::string::npos) {
      continue; // Skip lines without '='
    }

    const std::string rawKey = line.substr(0, sepPos);
    const std::string rawVal = line.substr(sepPos + 1);

    // Trim keys and values individually to handle "Key = Value"
    const std::string key = Trim(rawKey);
    const std::string val = Trim(rawVal);

    if (!key.empty()) {
      values_[key] = val;
    }
  }
}

double Config::GetDouble(const std::string &key) const {
  if (values_.find(key) == values_.end()) {
    throw std::runtime_error("Config: Missing key: " + key);
  }
  return std::stod(values_.at(key));
}

int Config::GetInt(const std::string &key) const {
  if (values_.find(key) == values_.end()) {
    throw std::runtime_error("Config: Missing key: " + key);
  }
  return std::stoi(values_.at(key));
}

std::string Config::GetString(const std::string &key) const {
  if (values_.find(key) == values_.end()) {
    throw std::runtime_error("Config: Missing key: " + key);
  }
  return values_.at(key);
}

} // namespace ArbSim