#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

#include "rule_item_type.h"

namespace dto
{

RuleItemType::RuleItemType(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json RuleItemType::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор правила
    if (ruleId.has_value())
    {
        result["ruleId"] = ruleId.value();
    }
    // Идентификатор типа элемента
    if (itemTypeId.has_value())
    {
        result["itemTypeId"] = itemTypeId.value();
    }
    // Право на чтение элементов данного типа
    if (isReader.has_value())
    {
        result["isReader"] = isReader.value();
    }
    // Право на создание и изменение элементов данного типа
    if (isWriter.has_value())
    {
        result["isWriter"] = isWriter.value();
    }

    return result;
}

bool RuleItemType::fromJson(const nlohmann::json& json)
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
    // Идентификатор правила
    if (json.contains("ruleId") && !json["ruleId"].is_null())
    {
        try
        {
            ruleId = json["ruleId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        ruleId = std::nullopt;
    }
    // Идентификатор типа элемента
    if (json.contains("itemTypeId") && !json["itemTypeId"].is_null())
    {
        try
        {
            itemTypeId = json["itemTypeId"].get<int64_t>();
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
    // Право на чтение элементов данного типа
    if (json.contains("isReader") && !json["isReader"].is_null())
    {
        try
        {
            isReader = json["isReader"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isReader = std::nullopt;
    }
    // Право на создание и изменение элементов данного типа
    if (json.contains("isWriter") && !json["isWriter"].is_null())
    {
        try
        {
            isWriter = json["isWriter"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isWriter = std::nullopt;
    }

    return success;
}

bool RuleItemType::isValid() const
{
    if (!ruleId.has_value())
    {
        return false;
    }
    if (!itemTypeId.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string RuleItemType::validationError() const
{
    if (!ruleId.has_value())
    {
        return "Поле «ruleId» является обязательным для заполнения";
    }
    if (!itemTypeId.has_value())
    {
        return "Поле «itemTypeId» является обязательным для заполнения";
    }


    return "";
}

bool RuleItemType::operator==(const RuleItemType& other) const
{
    return
        id == other.id
        && ruleId == other.ruleId
        && itemTypeId == other.itemTypeId
        && isReader == other.isReader
        && isWriter == other.isWriter
;
}

} // namespace dto