#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "standard_day.h"

namespace dto
{

StandardDay::StandardDay(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json StandardDay::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Номер дня недели (0-6)
    if (weekDayNumber.has_value())
    {
        result["week_day_number"] = weekDayNumber.value();
    }
    // Порядковый номер недели (для сменного графика)
    if (weekOrder.has_value())
    {
        result["week_order"] = weekOrder.value();
    }
    // Является ли день рабочим
    if (isWorkDay.has_value())
    {
        result["is_work_day"] = isWorkDay.value();
    }
    // Время начала работы (HH:MM)
    if (beginWorkTime.has_value())
    {
        result["begin_work_time"] = beginWorkTime.value();
    }
    // Время окончания работы (HH:MM)
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

bool StandardDay::fromJson(const nlohmann::json& json)
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
    // Номер дня недели (0-6)
    if (json.contains("week_day_number") && !json["week_day_number"].is_null())
    {
        try
        {
            weekDayNumber = json["week_day_number"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        weekDayNumber = std::nullopt;
    }
    // Порядковый номер недели (для сменного графика)
    if (json.contains("week_order") && !json["week_order"].is_null())
    {
        try
        {
            weekOrder = json["week_order"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        weekOrder = std::nullopt;
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
    // Время начала работы (HH:MM)
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
    // Время окончания работы (HH:MM)
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

bool StandardDay::isValid() const
{
    if (!weekDayNumber.has_value())
    {
        return false;
    }
    if (!weekOrder.has_value())
    {
        return false;
    }
    if (!isWorkDay.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string StandardDay::validationError() const
{
    if (!weekDayNumber.has_value())
    {
        return "Поле «week_day_number» является обязательным для заполнения";
    }
    if (!weekOrder.has_value())
    {
        return "Поле «week_order» является обязательным для заполнения";
    }
    if (!isWorkDay.has_value())
    {
        return "Поле «is_work_day» является обязательным для заполнения";
    }


    return "";
}

bool StandardDay::operator==(const StandardDay& other) const
{
    return
        id == other.id
        && weekDayNumber == other.weekDayNumber
        && weekOrder == other.weekOrder
        && isWorkDay == other.isWorkDay
        && beginWorkTime == other.beginWorkTime
        && endWorkTime == other.endWorkTime
        && breakDuration == other.breakDuration
;
}

} // namespace dto