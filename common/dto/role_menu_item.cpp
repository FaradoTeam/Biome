#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "role_menu_item.h"

namespace dto
{

RoleMenuItem::RoleMenuItem(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json RoleMenuItem::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор роли
    if (roleId.has_value())
    {
        result["role_id"] = roleId.value();
    }
    // Название пункта меню
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Ссылка (URL)
    if (link.has_value())
    {
        result["link"] = link.value();
    }
    // Значок пункта меню
    if (icon.has_value())
    {
        result["icon"] = icon.value();
    }

    return result;
}

bool RoleMenuItem::fromJson(const nlohmann::json& json)
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
    // Идентификатор роли
    if (json.contains("role_id") && !json["role_id"].is_null())
    {
        try
        {
            roleId = json["role_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        roleId = std::nullopt;
    }
    // Название пункта меню
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
    // Ссылка (URL)
    if (json.contains("link") && !json["link"].is_null())
    {
        try
        {
            link = json["link"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        link = std::nullopt;
    }
    // Значок пункта меню
    if (json.contains("icon") && !json["icon"].is_null())
    {
        try
        {
            icon = json["icon"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        icon = std::nullopt;
    }

    return success;
}

bool RoleMenuItem::isValid() const
{
    if (!roleId.has_value())
    {
        return false;
    }
    if (!caption.has_value())
    {
        return false;
    }
    if (!link.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений
    if (caption.value().empty())
    {
        return false;
    }
    if (link.value().empty())
    {
        return false;
    }

    return true;
}

std::string RoleMenuItem::validationError() const
{
    if (!roleId.has_value())
    {
        return "Поле «role_id» является обязательным для заполнения";
    }
    if (!caption.has_value())
    {
        return "Поле «caption» является обязательным для заполнения";
    }
    if (!link.has_value())
    {
        return "Поле «link» является обязательным для заполнения";
    }

    if (caption.value().empty())
    {
        return "Поле «caption» не может быть пустой строкой";
    }
    if (link.value().empty())
    {
        return "Поле «link» не может быть пустой строкой";
    }

    return "";
}

bool RoleMenuItem::operator==(const RoleMenuItem& other) const
{
    return
        id == other.id
        && roleId == other.roleId
        && caption == other.caption
        && link == other.link
        && icon == other.icon
;
}

} // namespace dto