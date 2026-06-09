#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

#include "private_message.h"

namespace dto
{

PrivateMessage::PrivateMessage(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json PrivateMessage::toJson() const
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
        result["senderUserId"] = senderUserId.value();
    }
    // Идентификатор получателя
    if (receiverUserId.has_value())
    {
        result["receiverUserId"] = receiverUserId.value();
    }
    // Время отправки
    if (creationTimestamp.has_value())
    {
        result["creationTimestamp"] = common::timePointToSeconds(creationTimestamp.value());
    }
    // Текст сообщения
    if (content.has_value())
    {
        result["content"] = content.value();
    }
    // Флаг прочтения
    if (isViewed.has_value())
    {
        result["isViewed"] = isViewed.value();
    }

    return result;
}

bool PrivateMessage::fromJson(const nlohmann::json& json)
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
    if (json.contains("senderUserId") && !json["senderUserId"].is_null())
    {
        try
        {
            senderUserId = json["senderUserId"].get<int64_t>();
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
    // Идентификатор получателя
    if (json.contains("receiverUserId") && !json["receiverUserId"].is_null())
    {
        try
        {
            receiverUserId = json["receiverUserId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        receiverUserId = std::nullopt;
    }
    // Время отправки
    if (json.contains("creationTimestamp") && !json["creationTimestamp"].is_null())
    {
        try
        {
            auto timestampValue = json["creationTimestamp"].get<int64_t>();
            creationTimestamp = common::secondsToTimePoint(timestampValue);
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
    // Флаг прочтения
    if (json.contains("isViewed") && !json["isViewed"].is_null())
    {
        try
        {
            isViewed = json["isViewed"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isViewed = std::nullopt;
    }

    return success;
}

bool PrivateMessage::isValid() const
{
    if (!senderUserId.has_value())
    {
        return false;
    }
    if (!receiverUserId.has_value())
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

std::string PrivateMessage::validationError() const
{
    if (!senderUserId.has_value())
    {
        return "Поле «senderUserId» является обязательным для заполнения";
    }
    if (!receiverUserId.has_value())
    {
        return "Поле «receiverUserId» является обязательным для заполнения";
    }
    if (!creationTimestamp.has_value())
    {
        return "Поле «creationTimestamp» является обязательным для заполнения";
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

bool PrivateMessage::operator==(const PrivateMessage& other) const
{
    return
        id == other.id
        && senderUserId == other.senderUserId
        && receiverUserId == other.receiverUserId
        && creationTimestamp == other.creationTimestamp
        && content == other.content
        && isViewed == other.isViewed
;
}

} // namespace dto