#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "field_type.h"

namespace dto
{

FieldType::FieldType(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json FieldType::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор типа элемента
    if (itemTypeId.has_value())
    {
        result["item_type_id"] = itemTypeId.value();
    }
    // Название поля
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Описание поля
    if (description.has_value())
    {
        result["description"] = description.value();
    }
    // Тип значения
    if (valueType.has_value())
    {
        result["value_type"] = valueType.value();
    }
    // Отображать поле на канбан-доске
    if (isBoardVisible.has_value())
    {
        result["is_board_visible"] = isBoardVisible.value();
    }

    return result;
}

bool FieldType::fromJson(const nlohmann::json& json)
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
    // Идентификатор типа элемента
    if (json.contains("item_type_id") && !json["item_type_id"].is_null())
    {
        try
        {
            itemTypeId = json["item_type_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        itemTypeId = std::nullopt;
    }
    // Название поля
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
    // Описание поля
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
    // Тип значения
    if (json.contains("value_type") && !json["value_type"].is_null())
    {
        try
        {
            valueType = json["value_type"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        valueType = std::nullopt;
    }
    // Отображать поле на канбан-доске
    if (json.contains("is_board_visible") && !json["is_board_visible"].is_null())
    {
        try
        {
            isBoardVisible = json["is_board_visible"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isBoardVisible = std::nullopt;
    }

    return success;
}

bool FieldType::isValid() const
{
    if (!itemTypeId.has_value())
    {
        return false;
    }
    if (!caption.has_value())
    {
        return false;
    }
    if (!valueType.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений
    if (caption.value().empty())
    {
        return false;
    }
    if (valueType.value().empty())
    {
        return false;
    }

    return true;
}

std::string FieldType::validationError() const
{
    if (!itemTypeId.has_value())
    {
        return "Поле «item_type_id» является обязательным для заполнения";
    }
    if (!caption.has_value())
    {
        return "Поле «caption» является обязательным для заполнения";
    }
    if (!valueType.has_value())
    {
        return "Поле «value_type» является обязательным для заполнения";
    }

    if (caption.value().empty())
    {
        return "Поле «caption» не может быть пустой строкой";
    }
    if (valueType.value().empty())
    {
        return "Поле «value_type» не может быть пустой строкой";
    }

    return "";
}

bool FieldType::operator==(const FieldType& other) const
{
    return
        id == other.id
        && itemTypeId == other.itemTypeId
        && caption == other.caption
        && description == other.description
        && valueType == other.valueType
        && isBoardVisible == other.isBoardVisible
;
}

} // namespace dto