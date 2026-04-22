#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

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
        result["link_type_id"] = linkTypeId.value();
    }
    // Исходный элемент
    if (sourceItemId.has_value())
    {
        result["source_item_id"] = sourceItemId.value();
    }
    // Целевой элемент
    if (destinationItemId.has_value())
    {
        result["destination_item_id"] = destinationItemId.value();
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
    if (json.contains("link_type_id") && !json["link_type_id"].is_null())
    {
        try
        {
            linkTypeId = json["link_type_id"].get<int64_t>();
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
    if (json.contains("source_item_id") && !json["source_item_id"].is_null())
    {
        try
        {
            sourceItemId = json["source_item_id"].get<int64_t>();
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
    if (json.contains("destination_item_id") && !json["destination_item_id"].is_null())
    {
        try
        {
            destinationItemId = json["destination_item_id"].get<int64_t>();
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
        return "Поле «link_type_id» является обязательным для заполнения";
    }
    if (!sourceItemId.has_value())
    {
        return "Поле «source_item_id» является обязательным для заполнения";
    }
    if (!destinationItemId.has_value())
    {
        return "Поле «destination_item_id» является обязательным для заполнения";
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