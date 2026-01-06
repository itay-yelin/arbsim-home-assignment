#pragma once
#include <string>
#include <unordered_map>

namespace ArbSim {

class Config {
public:
  explicit Config(const std::string &path);

  double GetDouble(const std::string &key) const;
  int GetInt(const std::string &key) const;
  std::string GetString(const std::string &key) const;

  // Returns a validated file path that is guaranteed to be within the allowed
  // base directory. Throws std::runtime_error if path escapes the base or
  // contains suspicious patterns.
  std::string GetValidatedPath(const std::string &key) const;

  // Sets the base directory for path validation. Defaults to current working
  // directory.
  void SetAllowedBaseDir(const std::string &baseDir);

private:
  std::unordered_map<std::string, std::string> values_;
  std::string allowedBaseDir_;

  // Validates that a path doesn't escape the allowed base directory
  bool IsPathSafe(const std::string &path) const;
};

} // namespace ArbSim