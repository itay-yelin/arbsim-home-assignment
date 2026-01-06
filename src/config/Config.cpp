#include "Config.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

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

Config::Config(const std::string &path)
    : allowedBaseDir_(fs::current_path().string()) {
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

void Config::SetAllowedBaseDir(const std::string &baseDir) {
  allowedBaseDir_ = fs::weakly_canonical(fs::path(baseDir)).string();
}

bool Config::IsPathSafe(const std::string &path) const {
  // Check for obvious path traversal patterns
  if (path.find("..") != std::string::npos) {
    return false;
  }

  // Resolve the path to canonical form
  fs::path inputPath(path);
  fs::path resolvedPath;

  try {
    // Use weakly_canonical to handle non-existent files
    if (inputPath.is_absolute()) {
      resolvedPath = fs::weakly_canonical(inputPath);
    } else {
      resolvedPath = fs::weakly_canonical(fs::path(allowedBaseDir_) / inputPath);
    }
  } catch (const fs::filesystem_error &) {
    return false;
  }

  // Get canonical base directory
  fs::path basePath = fs::weakly_canonical(fs::path(allowedBaseDir_));

  // Check if resolved path starts with base directory
  std::string resolvedStr = resolvedPath.string();
  std::string baseStr = basePath.string();

  // Normalize separators for comparison on Windows
#ifdef _WIN32
  std::replace(resolvedStr.begin(), resolvedStr.end(), '/', '\\');
  std::replace(baseStr.begin(), baseStr.end(), '/', '\\');
#endif

  // Ensure the resolved path is within the base directory
  if (resolvedStr.length() < baseStr.length()) {
    return false;
  }

  // Check prefix match
  if (resolvedStr.compare(0, baseStr.length(), baseStr) != 0) {
    return false;
  }

  // Ensure it's not just a prefix of a different directory name
  // e.g., /home/user vs /home/username
  if (resolvedStr.length() > baseStr.length()) {
    char nextChar = resolvedStr[baseStr.length()];
#ifdef _WIN32
    if (nextChar != '\\' && nextChar != '/') {
      return false;
    }
#else
    if (nextChar != '/') {
      return false;
    }
#endif
  }

  return true;
}

std::string Config::GetValidatedPath(const std::string &key) const {
  std::string path = GetString(key);

  if (!IsPathSafe(path)) {
    throw std::runtime_error(
        "Config: Path validation failed for key '" + key +
        "': path escapes allowed directory or contains invalid patterns. "
        "Path: " + path + ", Allowed base: " + allowedBaseDir_);
  }

  return path;
}

} // namespace ArbSim