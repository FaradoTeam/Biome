#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

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
        result["weekDayNumber"] = weekDayNumber.value();
    }
    // Порядковый номер недели (для сменного графика)
    if (weekOrder.has_value())
    {
        result["weekOrder"] = weekOrder.value();
    }
    // Является ли день рабочим
    if (isWorkDay.has_value())
    {
        result["isWorkDay"] = isWorkDay.value();
    }
    // Время начала работы (HH:MM)
    if (beginWorkTime.has_value())
    {
        result["beginWorkTime"] = beginWorkTime.value();
    }
    // Время окончания работы (HH:MM)
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
    if (json.contains("weekDayNumber") && !json["weekDayNumber"].is_null())
    {
        try
        {
            weekDayNumber = json["weekDayNumber"].get<int64_t>();
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
    if (json.contains("weekOrder") && !json["weekOrder"].is_null())
    {
        try
        {
            weekOrder = json["weekOrder"].get<int64_t>();
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
    // Время начала работы (HH:MM)
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
    // Время окончания работы (HH:MM)
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
        return "Поле «weekDayNumber» является обязательным для заполнения";
    }
    if (!weekOrder.has_value())
    {
        return "Поле «weekOrder» является обязательным для заполнения";
    }
    if (!isWorkDay.has_value())
    {
        return "Поле «isWorkDay» является обязательным для заполнения";
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