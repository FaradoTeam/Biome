#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace dto
{

// Вспомогательная функция для преобразования timePoint в секунды с момента
// начала эпохи.
inline int64_t timePointToSeconds(
    const std::chrono::system_clock::time_point& timePoint
)
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        timePoint.time_since_epoch()
    ).count();
}

// Вспомогательная функция для преобразования секунд, прошедших с момента
// начала эпохи, в timePoint.
inline std::chrono::system_clock::time_point secondsToTimePoint(int64_t seconds)
{
    return std::chrono::system_clock::from_time_t(static_cast<time_t>(seconds));
}

} // namespace dto
