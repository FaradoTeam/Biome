#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

#include "project_team.h"

namespace dto
{

ProjectTeam::ProjectTeam(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json ProjectTeam::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор проекта
    if (projectId.has_value())
    {
        result["projectId"] = projectId.value();
    }
    // Идентификатор команды
    if (teamId.has_value())
    {
        result["teamId"] = teamId.value();
    }

    return result;
}

bool ProjectTeam::fromJson(const nlohmann::json& json)
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
    // Идентификатор проекта
    if (json.contains("projectId") && !json["projectId"].is_null())
    {
        try
        {
            projectId = json["projectId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        projectId = std::nullopt;
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

    return success;
}

bool ProjectTeam::isValid() const
{
    if (!projectId.has_value())
    {
        return false;
    }
    if (!teamId.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string ProjectTeam::validationError() const
{
    if (!projectId.has_value())
    {
        return "Поле «projectId» является обязательным для заполнения";
    }
    if (!teamId.has_value())
    {
        return "Поле «teamId» является обязательным для заполнения";
    }


    return "";
}

bool ProjectTeam::operator==(const ProjectTeam& other) const
{
    return
        id == other.id
        && projectId == other.projectId
        && teamId == other.teamId
;
}

} // namespace dto