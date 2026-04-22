#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "user_notification.h"

namespace dto
{

UserNotification::UserNotification(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json UserNotification::toJson() const
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
    // Идентификатор элемента
    if (itemId.has_value())
    {
        result["item_id"] = itemId.value();
    }

    return result;
}

bool UserNotification::fromJson(const nlohmann::json& json)
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

    return success;
}

bool UserNotification::isValid() const
{
    if (!userId.has_value())
    {
        return false;
    }
    if (!itemId.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string UserNotification::validationError() const
{
    if (!userId.has_value())
    {
        return "Поле «user_id» является обязательным для заполнения";
    }
    if (!itemId.has_value())
    {
        return "Поле «item_id» является обязательным для заполнения";
    }


    return "";
}

bool UserNotification::operator==(const UserNotification& other) const
{
    return
        id == other.id
        && userId == other.userId
        && itemId == other.itemId
;
}

} // namespace dto