#include "Config.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

Config::Config(const std::string& path)
{
    std::ifstream f(path);
    if (!f)
        throw std::runtime_error("Config: Failed to open file: " + path);

    std::string line;
    while (std::getline(f, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        values_[line.substr(0, pos)] = line.substr(pos + 1);
    }
}

double Config::GetDouble(const std::string& key) const
{
    return std::stod(values_.at(key));
}

int Config::GetInt(const std::string& key) const
{
    return std::stoi(values_.at(key));
}

std::string Config::GetString(const std::string& key) const
{
    return values_.at(key);
}
