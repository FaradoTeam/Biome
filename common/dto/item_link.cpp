#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

#include "item_link.h"

namespace dto
{

ItemLink::ItemLink(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json ItemLink::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор типа связи
    if (linkTypeId.has_value())
    {
        result["linkTypeId"] = linkTypeId.value();
    }
    // Исходный элемент
    if (sourceItemId.has_value())
    {
        result["sourceItemId"] = sourceItemId.value();
    }
    // Целевой элемент
    if (destinationItemId.has_value())
    {
        result["destinationItemId"] = destinationItemId.value();
    }

    return result;
}

bool ItemLink::fromJson(const nlohmann::json& json)
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
    // Идентификатор типа связи
    if (json.contains("linkTypeId") && !json["linkTypeId"].is_null())
    {
        try
        {
            linkTypeId = json["linkTypeId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        linkTypeId = std::nullopt;
    }
    // Исходный элемент
    if (json.contains("sourceItemId") && !json["sourceItemId"].is_null())
    {
        try
        {
            sourceItemId = json["sourceItemId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        sourceItemId = std::nullopt;
    }
    // Целевой элемент
    if (json.contains("destinationItemId") && !json["destinationItemId"].is_null())
    {
        try
        {
            destinationItemId = json["destinationItemId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        destinationItemId = std::nullopt;
    }

    return success;
}

bool ItemLink::isValid() const
{
    if (!linkTypeId.has_value())
    {
        return false;
    }
    if (!sourceItemId.has_value())
    {
        return false;
    }
    if (!destinationItemId.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string ItemLink::validationError() const
{
    if (!linkTypeId.has_value())
    {
        return "Поле «linkTypeId» является обязательным для заполнения";
    }
    if (!sourceItemId.has_value())
    {
        return "Поле «sourceItemId» является обязательным для заполнения";
    }
    if (!destinationItemId.has_value())
    {
        return "Поле «destinationItemId» является обязательным для заполнения";
    }


    return "";
}

bool ItemLink::operator==(const ItemLink& other) const
{
    return
        id == other.id
        && linkTypeId == other.linkTypeId
        && sourceItemId == other.sourceItemId
        && destinationItemId == other.destinationItemId
;
}

} // namespace dto