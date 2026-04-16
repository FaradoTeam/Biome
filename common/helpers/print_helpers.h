#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace dto
{

inline std::string timePointToString(const std::chrono::system_clock::time_point& timePoint)
{
    const auto time = std::chrono::system_clock::to_time_t(timePoint);
    std::stringstream result;
    result << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return result.str();
}

} // namespace dto

namespace std
{
namespace chrono
{

inline ostream& operator<<(ostream& stream, const system_clock::time_point& timePoint)
{
    stream << dto::timePointToString(timePoint);
    return stream;
}

} // namespace chrono
} // namespace std
