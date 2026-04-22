#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "user_todo.h"

namespace dto
{

UserTodo::UserTodo(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json UserTodo::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор элемента
    if (itemId.has_value())
    {
        result["item_id"] = itemId.value();
    }
    // Идентификатор пользователя
    if (userId.has_value())
    {
        result["user_id"] = userId.value();
    }
    // Плановая дата начала
    if (startDate.has_value())
    {
        result["start_date"] = timePointToSeconds(startDate.value());
    }
    // Плановая дата окончания
    if (endDate.has_value())
    {
        result["end_date"] = timePointToSeconds(endDate.value());
    }

    return result;
}

bool UserTodo::fromJson(const nlohmann::json& json)
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
    // Идентификатор элемента
    if (json.contains("item_id") && !json["item_id"].is_null())
    {
        try
        {
            itemId = json["item_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        itemId = std::nullopt;
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
    // Плановая дата начала
    if (json.contains("start_date") && !json["start_date"].is_null())
    {
        try
        {
            auto timestampValue = json["start_date"].get<int64_t>();
            startDate = secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        startDate = std::nullopt;
    }
    // Плановая дата окончания
    if (json.contains("end_date") && !json["end_date"].is_null())
    {
        try
        {
            auto timestampValue = json["end_date"].get<int64_t>();
            endDate = secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        endDate = std::nullopt;
    }

    return success;
}

bool UserTodo::isValid() const
{
    if (!itemId.has_value())
    {
        return false;
    }
    if (!userId.has_value())
    {
        return false;
    }
    if (!startDate.has_value())
    {
        return false;
    }
    if (!endDate.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string UserTodo::validationError() const
{
    if (!itemId.has_value())
    {
        return "Поле «item_id» является обязательным для заполнения";
    }
    if (!userId.has_value())
    {
        return "Поле «user_id» является обязательным для заполнения";
    }
    if (!startDate.has_value())
    {
        return "Поле «start_date» является обязательным для заполнения";
    }
    if (!endDate.has_value())
    {
        return "Поле «end_date» является обязательным для заполнения";
    }


    return "";
}

bool UserTodo::operator==(const UserTodo& other) const
{
    return
        id == other.id
        && itemId == other.itemId
        && userId == other.userId
        && startDate == other.startDate
        && endDate == other.endDate
;
}

} // namespace dto