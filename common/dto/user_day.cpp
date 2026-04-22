#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "user_day.h"

namespace dto
{

UserDay::UserDay(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json UserDay::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор пользователя
    if (userId.has_value())
    {
        result["user_id"] = userId.value();
    }
    // Дата (уникальна в паре с userId)
    if (date.has_value())
    {
        result["date"] = timePointToSeconds(date.value());
    }
    // Является ли день рабочим для пользователя
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
    // Причина (отпуск
    if (description.has_value())
    {
        result["description"] = description.value();
    }

    return result;
}

bool UserDay::fromJson(const nlohmann::json& json)
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
    // Идентификатор пользователя
    if (json.contains("user_id") && !json["user_id"].is_null())
    {
        try
        {
            userId = json["user_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        userId = std::nullopt;
    }
    // Дата (уникальна в паре с userId)
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
    // Является ли день рабочим для пользователя
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
    // Причина (отпуск
    if (json.contains("description") && !json["description"].is_null())
    {
        try
        {
            description = json["description"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        description = std::nullopt;
    }

    return success;
}

bool UserDay::isValid() const
{
    if (!userId.has_value())
    {
        return false;
    }
    if (!date.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string UserDay::validationError() const
{
    if (!userId.has_value())
    {
        return "Поле «user_id» является обязательным для заполнения";
    }
    if (!date.has_value())
    {
        return "Поле «date» является обязательным для заполнения";
    }


    return "";
}

bool UserDay::operator==(const UserDay& other) const
{
    return
        id == other.id
        && userId == other.userId
        && date == other.date
        && isWorkDay == other.isWorkDay
        && beginWorkTime == other.beginWorkTime
        && endWorkTime == other.endWorkTime
        && breakDuration == other.breakDuration
        && description == other.description
;
}

} // namespace dto