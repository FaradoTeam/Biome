#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "field_type_possible_value.h"

namespace dto
{

FieldTypePossibleValue::FieldTypePossibleValue(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json FieldTypePossibleValue::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор типа поля
    if (fieldTypeId.has_value())
    {
        result["field_type_id"] = fieldTypeId.value();
    }
    // Значение для выбора
    if (value.has_value())
    {
        result["value"] = value.value();
    }

    return result;
}

bool FieldTypePossibleValue::fromJson(const nlohmann::json& json)
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
    // Значение для выбора
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

bool FieldTypePossibleValue::isValid() const
{
    if (!fieldTypeId.has_value())
    {
        return false;
    }
    if (!value.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений
    if (value.value().empty())
    {
        return false;
    }

    return true;
}

std::string FieldTypePossibleValue::validationError() const
{
    if (!fieldTypeId.has_value())
    {
        return "Поле «field_type_id» является обязательным для заполнения";
    }
    if (!value.has_value())
    {
        return "Поле «value» является обязательным для заполнения";
    }

    if (value.value().empty())
    {
        return "Поле «value» не может быть пустой строкой";
    }

    return "";
}

bool FieldTypePossibleValue::operator==(const FieldTypePossibleValue& other) const
{
    return
        id == other.id
        && fieldTypeId == other.fieldTypeId
        && value == other.value
;
}

} // namespace dto