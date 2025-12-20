#pragma once
#include <string>
#include <unordered_map>

class Config
{
public:
    explicit Config(const std::string& path);

    double GetDouble(const std::string& key) const;
    int    GetInt(const std::string& key) const;
    std::string GetString(const std::string& key) const;

private:
    std::unordered_map<std::string, std::string> values_;
};