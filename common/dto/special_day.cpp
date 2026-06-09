#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

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
        result["date"] = common::timePointToSeconds(date.value());
    }
    // Является ли день рабочим
    if (isWorkDay.has_value())
    {
        result["isWorkDay"] = isWorkDay.value();
    }
    // Время начала работы (если рабочий)
    if (beginWorkTime.has_value())
    {
        result["beginWorkTime"] = beginWorkTime.value();
    }
    // Время окончания работы (если рабочий)
    if (endWorkTime.has_value())
    {
        result["endWorkTime"] = endWorkTime.value();
    }
    // Длительность перерыва в минутах
    if (breakDuration.has_value())
    {
        result["breakDuration"] = breakDuration.value();
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
            date = common::secondsToTimePoint(timestampValue);
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
    if (json.contains("isWorkDay") && !json["isWorkDay"].is_null())
    {
        try
        {
            isWorkDay = json["isWorkDay"].get<bool>();
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
    if (json.contains("beginWorkTime") && !json["beginWorkTime"].is_null())
    {
        try
        {
            beginWorkTime = json["beginWorkTime"].get<std::string>();
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
    if (json.contains("endWorkTime") && !json["endWorkTime"].is_null())
    {
        try
        {
            endWorkTime = json["endWorkTime"].get<std::string>();
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
    if (json.contains("breakDuration") && !json["breakDuration"].is_null())
    {
        try
        {
            breakDuration = json["breakDuration"].get<int64_t>();
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