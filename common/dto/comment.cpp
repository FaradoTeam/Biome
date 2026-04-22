#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

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
        result["user_id"] = userId.value();
    }
    // Идентификатор элемента
    if (itemId.has_value())
    {
        result["item_id"] = itemId.value();
    }
    // Время создания
    if (createdAt.has_value())
    {
        result["created_at"] = timePointToSeconds(createdAt.value());
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
    // Время создания
    if (json.contains("created_at") && !json["created_at"].is_null())
    {
        try
        {
            auto timestampValue = json["created_at"].get<int64_t>();
            createdAt = secondsToTimePoint(timestampValue);
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
        return "Поле «user_id» является обязательным для заполнения";
    }
    if (!itemId.has_value())
    {
        return "Поле «item_id» является обязательным для заполнения";
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