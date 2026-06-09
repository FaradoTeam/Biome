#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

#include "comment.h"

namespace dto
{

Comment::Comment(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json Comment::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор автора
    if (userId.has_value())
    {
        result["userId"] = userId.value();
    }
    // Идентификатор элемента
    if (itemId.has_value())
    {
        result["itemId"] = itemId.value();
    }
    // Время создания
    if (createdAt.has_value())
    {
        result["createdAt"] = common::timePointToSeconds(createdAt.value());
    }
    // Текст комментария
    if (content.has_value())
    {
        result["content"] = content.value();
    }

    return result;
}

bool Comment::fromJson(const nlohmann::json& json)
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
    // Идентификатор автора
    if (json.contains("userId") && !json["userId"].is_null())
    {
        try
        {
            userId = json["userId"].get<int64_t>();
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
    if (json.contains("itemId") && !json["itemId"].is_null())
    {
        try
        {
            itemId = json["itemId"].get<int64_t>();
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
    // Время создания
    if (json.contains("createdAt") && !json["createdAt"].is_null())
    {
        try
        {
            auto timestampValue = json["createdAt"].get<int64_t>();
            createdAt = common::secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        createdAt = std::nullopt;
    }
    // Текст комментария
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

bool Comment::isValid() const
{
    if (!userId.has_value())
    {
        return false;
    }
    if (!itemId.has_value())
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

std::string Comment::validationError() const
{
    if (!userId.has_value())
    {
        return "Поле «userId» является обязательным для заполнения";
    }
    if (!itemId.has_value())
    {
        return "Поле «itemId» является обязательным для заполнения";
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

bool Comment::operator==(const Comment& other) const
{
    return
        id == other.id
        && userId == other.userId
        && itemId == other.itemId
        && createdAt == other.createdAt
        && content == other.content
;
}

} // namespace dto