#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace common
{

/**
 * @brief Тип для хранения даты/времени (момент времени в UTC).
 */
using DateTime = std::chrono::system_clock::time_point;

/**
 * @brief Тип для хранения бинарных данных (BLOB).
 */
using Blob = std::vector<uint8_t>;

/**
 * @brief Тип для идентификаторов сущностей.
 */
using Id = int64_t;

/**
 * @brief Вспомогательная функция для преобразования time_point в секунды.
 */
inline int64_t timePointToSeconds(const DateTime& timePoint)
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        timePoint.time_since_epoch()
    ).count();
}

/**
 * @brief Вспомогательная функция для преобразования секунд в time_point.
 */
inline DateTime secondsToTimePoint(int64_t seconds)
{
    return DateTime(std::chrono::seconds(seconds));
}

/**
 * @brief Вспомогательная функция для преобразования time_point в строку ISO8601.
 */
inline std::string dateTimeToString(const DateTime& dt)
{
    auto tt = std::chrono::system_clock::to_time_t(dt);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&tt));
    return buf;
}

/**
 * @brief Вспомогательная функция для преобразования строки в DateTime.
 */
inline DateTime stringToDateTime(const std::string& str)
{
    std::tm tm = {};
    strptime(str.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

} // namespace common
