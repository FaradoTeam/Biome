#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

#include "link_type.h"

namespace dto
{

LinkType::LinkType(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json LinkType::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Исходный тип элемента
    if (sourceItemTypeId.has_value())
    {
        result["sourceItemTypeId"] = sourceItemTypeId.value();
    }
    // Целевой тип элемента
    if (destinationItemTypeId.has_value())
    {
        result["destinationItemTypeId"] = destinationItemTypeId.value();
    }
    // Флаг двунаправленной связи
    if (isBidirectional.has_value())
    {
        result["isBidirectional"] = isBidirectional.value();
    }
    // Название типа связи
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }

    return result;
}

bool LinkType::fromJson(const nlohmann::json& json)
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
    // Исходный тип элемента
    if (json.contains("sourceItemTypeId") && !json["sourceItemTypeId"].is_null())
    {
        try
        {
            sourceItemTypeId = json["sourceItemTypeId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        sourceItemTypeId = std::nullopt;
    }
    // Целевой тип элемента
    if (json.contains("destinationItemTypeId") && !json["destinationItemTypeId"].is_null())
    {
        try
        {
            destinationItemTypeId = json["destinationItemTypeId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        destinationItemTypeId = std::nullopt;
    }
    // Флаг двунаправленной связи
    if (json.contains("isBidirectional") && !json["isBidirectional"].is_null())
    {
        try
        {
            isBidirectional = json["isBidirectional"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isBidirectional = std::nullopt;
    }
    // Название типа связи
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

bool LinkType::isValid() const
{
    if (!sourceItemTypeId.has_value())
    {
        return false;
    }
    if (!destinationItemTypeId.has_value())
    {
        return false;
    }
    if (!isBidirectional.has_value())
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

std::string LinkType::validationError() const
{
    if (!sourceItemTypeId.has_value())
    {
        return "Поле «sourceItemTypeId» является обязательным для заполнения";
    }
    if (!destinationItemTypeId.has_value())
    {
        return "Поле «destinationItemTypeId» является обязательным для заполнения";
    }
    if (!isBidirectional.has_value())
    {
        return "Поле «isBidirectional» является обязательным для заполнения";
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

bool LinkType::operator==(const LinkType& other) const
{
    return
        id == other.id
        && sourceItemTypeId == other.sourceItemTypeId
        && destinationItemTypeId == other.destinationItemTypeId
        && isBidirectional == other.isBidirectional
        && caption == other.caption
;
}

} // namespace dto