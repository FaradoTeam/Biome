#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

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
    // Идентификатор пользователя
    if (userId.has_value())
    {
        result["userId"] = userId.value();
    }
    // Флаг выполнения
    if (isDone.has_value())
    {
        result["isDone"] = isDone.value();
    }
    // Текст задачи
    if (caption.has_value())
    {
        result["caption"] = caption.value();
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
    // Идентификатор пользователя
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
    // Флаг выполнения
    if (json.contains("isDone") && !json["isDone"].is_null())
    {
        try
        {
            isDone = json["isDone"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isDone = std::nullopt;
    }
    // Текст задачи
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

    return success;
}

bool UserTodo::isValid() const
{
    if (!userId.has_value())
    {
        return false;
    }
    if (!isDone.has_value())
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

std::string UserTodo::validationError() const
{
    if (!userId.has_value())
    {
        return "Поле «userId» является обязательным для заполнения";
    }
    if (!isDone.has_value())
    {
        return "Поле «isDone» является обязательным для заполнения";
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

bool UserTodo::operator==(const UserTodo& other) const
{
    return
        id == other.id
        && userId == other.userId
        && isDone == other.isDone
        && caption == other.caption
;
}

} // namespace dto