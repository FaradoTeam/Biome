#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "rule_state.h"

namespace dto
{

RuleState::RuleState(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json RuleState::toJson() const
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
        result["rule_id"] = ruleId.value();
    }
    // Идентификатор целевого состояния
    if (stateId.has_value())
    {
        result["state_id"] = stateId.value();
    }
    // Разрешение на перевод элемента в это состояние
    if (isStateAllowed.has_value())
    {
        result["is_state_allowed"] = isStateAllowed.value();
    }

    return result;
}

bool RuleState::fromJson(const nlohmann::json& json)
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
    if (json.contains("rule_id") && !json["rule_id"].is_null())
    {
        try
        {
            ruleId = json["rule_id"].get<int64_t>();
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
    // Идентификатор целевого состояния
    if (json.contains("state_id") && !json["state_id"].is_null())
    {
        try
        {
            stateId = json["state_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        stateId = std::nullopt;
    }
    // Разрешение на перевод элемента в это состояние
    if (json.contains("is_state_allowed") && !json["is_state_allowed"].is_null())
    {
        try
        {
            isStateAllowed = json["is_state_allowed"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isStateAllowed = std::nullopt;
    }

    return success;
}

bool RuleState::isValid() const
{
    if (!ruleId.has_value())
    {
        return false;
    }
    if (!stateId.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string RuleState::validationError() const
{
    if (!ruleId.has_value())
    {
        return "Поле «rule_id» является обязательным для заполнения";
    }
    if (!stateId.has_value())
    {
        return "Поле «state_id» является обязательным для заполнения";
    }


    return "";
}

bool RuleState::operator==(const RuleState& other) const
{
    return
        id == other.id
        && ruleId == other.ruleId
        && stateId == other.stateId
        && isStateAllowed == other.isStateAllowed
;
}

} // namespace dto