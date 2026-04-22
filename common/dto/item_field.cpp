#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "item_field.h"

namespace dto
{

ItemField::ItemField(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json ItemField::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор элемента
    if (itemId.has_value())
    {
        result["item_id"] = itemId.value();
    }
    // Идентификатор типа поля
    if (fieldTypeId.has_value())
    {
        result["field_type_id"] = fieldTypeId.value();
    }
    // Значение поля (в сериализованном виде)
    if (value.has_value())
    {
        result["value"] = value.value();
    }

    return result;
}

bool ItemField::fromJson(const nlohmann::json& json)
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
    // Идентификатор типа поля
    if (json.contains("field_type_id") && !json["field_type_id"].is_null())
    {
        try
        {
            fieldTypeId = json["field_type_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        fieldTypeId = std::nullopt;
    }
    // Значение поля (в сериализованном виде)
    if (json.contains("value") && !json["value"].is_null())
    {
        try
        {
            value = json["value"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        value = std::nullopt;
    }

    return success;
}

bool ItemField::isValid() const
{
    if (!itemId.has_value())
    {
        return false;
    }
    if (!fieldTypeId.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string ItemField::validationError() const
{
    if (!itemId.has_value())
    {
        return "Поле «item_id» является обязательным для заполнения";
    }
    if (!fieldTypeId.has_value())
    {
        return "Поле «field_type_id» является обязательным для заполнения";
    }


    return "";
}

bool ItemField::operator==(const ItemField& other) const
{
    return
        id == other.id
        && itemId == other.itemId
        && fieldTypeId == other.fieldTypeId
        && value == other.value
;
}

} // namespace dto