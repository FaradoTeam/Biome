#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "rule.h"

namespace dto
{

Rule::Rule(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json Rule::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор правила
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор роли
    if (roleId.has_value())
    {
        result["role_id"] = roleId.value();
    }
    // Право на создание корневых проектов
    if (isRootProjectCreator.has_value())
    {
        result["is_root_project_creator"] = isRootProjectCreator.value();
    }

    return result;
}

bool Rule::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // Уникальный идентификатор правила
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
    // Право на создание корневых проектов
    if (json.contains("is_root_project_creator") && !json["is_root_project_creator"].is_null())
    {
        try
        {
            isRootProjectCreator = json["is_root_project_creator"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isRootProjectCreator = std::nullopt;
    }

    return success;
}

bool Rule::isValid() const
{
    if (!roleId.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string Rule::validationError() const
{
    if (!roleId.has_value())
    {
        return "Поле «role_id» является обязательным для заполнения";
    }


    return "";
}

bool Rule::operator==(const Rule& other) const
{
    return
        id == other.id
        && roleId == other.roleId
        && isRootProjectCreator == other.isRootProjectCreator
;
}

} // namespace dto