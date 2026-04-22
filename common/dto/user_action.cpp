#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "user_action.h"

namespace dto
{

UserAction::UserAction(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json UserAction::toJson() const
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
    // Время действия
    if (timestamp.has_value())
    {
        result["timestamp"] = timePointToSeconds(timestamp.value());
    }
    // Краткое описание действия
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Подробное описание (контекст)
    if (description.has_value())
    {
        result["description"] = description.value();
    }

    return result;
}

bool UserAction::fromJson(const nlohmann::json& json)
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
    // Время действия
    if (json.contains("timestamp") && !json["timestamp"].is_null())
    {
        try
        {
            auto timestampValue = json["timestamp"].get<int64_t>();
            timestamp = secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        timestamp = std::nullopt;
    }
    // Краткое описание действия
    if (json.contains("caption") && !json["caption"].is_null())
    {
        try
        {
            caption = json["caption"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        caption = std::nullopt;
    }
    // Подробное описание (контекст)
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

bool UserAction::isValid() const
{
    if (!userId.has_value())
    {
        return false;
    }
    if (!timestamp.has_value())
    {
        return false;
    }
    if (!caption.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений
    if (caption.value().empty())
    {
        return false;
    }

    return true;
}

std::string UserAction::validationError() const
{
    if (!userId.has_value())
    {
        return "Поле «user_id» является обязательным для заполнения";
    }
    if (!timestamp.has_value())
    {
        return "Поле «timestamp» является обязательным для заполнения";
    }
    if (!caption.has_value())
    {
        return "Поле «caption» является обязательным для заполнения";
    }

    if (caption.value().empty())
    {
        return "Поле «caption» не может быть пустой строкой";
    }

    return "";
}

bool UserAction::operator==(const UserAction& other) const
{
    return
        id == other.id
        && userId == other.userId
        && timestamp == other.timestamp
        && caption == other.caption
        && description == other.description
;
}

} // namespace dto