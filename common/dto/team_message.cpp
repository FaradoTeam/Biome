#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "team_message.h"

namespace dto
{

TeamMessage::TeamMessage(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json TeamMessage::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор отправителя
    if (senderUserId.has_value())
    {
        result["sender_user_id"] = senderUserId.value();
    }
    // Идентификатор команды
    if (teamId.has_value())
    {
        result["team_id"] = teamId.value();
    }
    // Время отправки
    if (creationTimestamp.has_value())
    {
        result["creation_timestamp"] = timePointToSeconds(creationTimestamp.value());
    }
    // Текст сообщения
    if (content.has_value())
    {
        result["content"] = content.value();
    }

    return result;
}

bool TeamMessage::fromJson(const nlohmann::json& json)
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
    // Идентификатор отправителя
    if (json.contains("sender_user_id") && !json["sender_user_id"].is_null())
    {
        try
        {
            senderUserId = json["sender_user_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        senderUserId = std::nullopt;
    }
    // Идентификатор команды
    if (json.contains("team_id") && !json["team_id"].is_null())
    {
        try
        {
            teamId = json["team_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        teamId = std::nullopt;
    }
    // Время отправки
    if (json.contains("creation_timestamp") && !json["creation_timestamp"].is_null())
    {
        try
        {
            auto timestampValue = json["creation_timestamp"].get<int64_t>();
            creationTimestamp = secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        creationTimestamp = std::nullopt;
    }
    // Текст сообщения
    if (json.contains("content") && !json["content"].is_null())
    {
        try
        {
            content = json["content"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        content = std::nullopt;
    }

    return success;
}

bool TeamMessage::isValid() const
{
    if (!senderUserId.has_value())
    {
        return false;
    }
    if (!teamId.has_value())
    {
        return false;
    }
    if (!creationTimestamp.has_value())
    {
        return false;
    }
    if (!content.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений
    if (content.value().empty())
    {
        return false;
    }

    return true;
}

std::string TeamMessage::validationError() const
{
    if (!senderUserId.has_value())
    {
        return "Поле «sender_user_id» является обязательным для заполнения";
    }
    if (!teamId.has_value())
    {
        return "Поле «team_id» является обязательным для заполнения";
    }
    if (!creationTimestamp.has_value())
    {
        return "Поле «creation_timestamp» является обязательным для заполнения";
    }
    if (!content.has_value())
    {
        return "Поле «content» является обязательным для заполнения";
    }

    if (content.value().empty())
    {
        return "Поле «content» не может быть пустой строкой";
    }

    return "";
}

bool TeamMessage::operator==(const TeamMessage& other) const
{
    return
        id == other.id
        && senderUserId == other.senderUserId
        && teamId == other.teamId
        && creationTimestamp == other.creationTimestamp
        && content == other.content
;
}

} // namespace dto