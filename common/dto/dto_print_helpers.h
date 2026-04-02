#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace farado
{
namespace dto
{

inline std::string timePointToString(const std::chrono::system_clock::time_point& tp)
{
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

} // namespace dto
} // namespace farado

namespace std
{
namespace chrono
{

inline ostream& operator<<(ostream& os, const system_clock::time_point& tp)
{
    os << farado::dto::timePointToString(tp);
    return os;
}

} // namespace chrono
} // namespace std
