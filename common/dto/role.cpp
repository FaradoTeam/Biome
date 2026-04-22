#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "role.h"

namespace dto
{

Role::Role(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json Role::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор роли
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Название роли
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Описание роли
    if (description.has_value())
    {
        result["description"] = description.value();
    }

    return result;
}

bool Role::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // Уникальный идентификатор роли
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
    // Название роли
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
    // Описание роли
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

bool Role::isValid() const
{
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

std::string Role::validationError() const
{
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

bool Role::operator==(const Role& other) const
{
    return
        id == other.id
        && caption == other.caption
        && description == other.description
;
}

} // namespace dto