#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "special_day.h"

namespace dto
{

SpecialDay::SpecialDay(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json SpecialDay::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Дата (уникальная)
    if (date.has_value())
    {
        result["date"] = timePointToSeconds(date.value());
    }
    // Является ли день рабочим
    if (isWorkDay.has_value())
    {
        result["is_work_day"] = isWorkDay.value();
    }
    // Время начала работы (если рабочий)
    if (beginWorkTime.has_value())
    {
        result["begin_work_time"] = beginWorkTime.value();
    }
    // Время окончания работы (если рабочий)
    if (endWorkTime.has_value())
    {
        result["end_work_time"] = endWorkTime.value();
    }
    // Длительность перерыва в минутах
    if (breakDuration.has_value())
    {
        result["break_duration"] = breakDuration.value();
    }

    return result;
}

bool SpecialDay::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // Уникальный идентификатор
    if (json.contains("id") && !json["id"].is_null())
    {
        try
        {
            id = json["id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        id = std::nullopt;
    }
    // Дата (уникальная)
    if (json.contains("date") && !json["date"].is_null())
    {
        try
        {
            auto timestampValue = json["date"].get<int64_t>();
            date = secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        date = std::nullopt;
    }
    // Является ли день рабочим
    if (json.contains("is_work_day") && !json["is_work_day"].is_null())
    {
        try
        {
            isWorkDay = json["is_work_day"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isWorkDay = std::nullopt;
    }
    // Время начала работы (если рабочий)
    if (json.contains("begin_work_time") && !json["begin_work_time"].is_null())
    {
        try
        {
            beginWorkTime = json["begin_work_time"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        beginWorkTime = std::nullopt;
    }
    // Время окончания работы (если рабочий)
    if (json.contains("end_work_time") && !json["end_work_time"].is_null())
    {
        try
        {
            endWorkTime = json["end_work_time"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        endWorkTime = std::nullopt;
    }
    // Длительность перерыва в минутах
    if (json.contains("break_duration") && !json["break_duration"].is_null())
    {
        try
        {
            breakDuration = json["break_duration"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        breakDuration = std::nullopt;
    }

    return success;
}

bool SpecialDay::isValid() const
{
    if (!date.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string SpecialDay::validationError() const
{
    if (!date.has_value())
    {
        return "Поле «date» является обязательным для заполнения";
    }


    return "";
}

bool SpecialDay::operator==(const SpecialDay& other) const
{
    return
        id == other.id
        && date == other.date
        && isWorkDay == other.isWorkDay
        && beginWorkTime == other.beginWorkTime
        && endWorkTime == other.endWorkTime
        && breakDuration == other.breakDuration
;
}

} // namespace dto