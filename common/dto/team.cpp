#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "team.h"

namespace dto
{

Team::Team(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json Team::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор команды
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Название команды
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Описание команды
    if (description.has_value())
    {
        result["description"] = description.value();
    }

    return result;
}

bool Team::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // Уникальный идентификатор команды
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
    // Название команды
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
    // Описание команды
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

bool Team::isValid() const
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

std::string Team::validationError() const
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

bool Team::operator==(const Team& other) const
{
    return
        id == other.id
        && caption == other.caption
        && description == other.description
;
}

} // namespace dto