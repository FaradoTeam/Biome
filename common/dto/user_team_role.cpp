#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

#include "user_team_role.h"

namespace dto
{

UserTeamRole::UserTeamRole(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json UserTeamRole::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор пользователя
    if (userId.has_value())
    {
        result["userId"] = userId.value();
    }
    // Идентификатор команды
    if (teamId.has_value())
    {
        result["teamId"] = teamId.value();
    }
    // Идентификатор роли
    if (roleId.has_value())
    {
        result["roleId"] = roleId.value();
    }

    return result;
}

bool UserTeamRole::fromJson(const nlohmann::json& json)
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
    // Идентификатор пользователя
    if (json.contains("userId") && !json["userId"].is_null())
    {
        try
        {
            userId = json["userId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        userId = std::nullopt;
    }
    // Идентификатор команды
    if (json.contains("teamId") && !json["teamId"].is_null())
    {
        try
        {
            teamId = json["teamId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        teamId = std::nullopt;
    }
    // Идентификатор роли
    if (json.contains("roleId") && !json["roleId"].is_null())
    {
        try
        {
            roleId = json["roleId"].get<int64_t>();
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

    return success;
}

bool UserTeamRole::isValid() const
{
    if (!userId.has_value())
    {
        return false;
    }
    if (!teamId.has_value())
    {
        return false;
    }
    if (!roleId.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string UserTeamRole::validationError() const
{
    if (!userId.has_value())
    {
        return "Поле «userId» является обязательным для заполнения";
    }
    if (!teamId.has_value())
    {
        return "Поле «teamId» является обязательным для заполнения";
    }
    if (!roleId.has_value())
    {
        return "Поле «roleId» является обязательным для заполнения";
    }


    return "";
}

bool UserTeamRole::operator==(const UserTeamRole& other) const
{
    return
        id == other.id
        && userId == other.userId
        && teamId == other.teamId
        && roleId == other.roleId
;
}

} // namespace dto