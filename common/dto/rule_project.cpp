#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

#include "rule_project.h"

namespace dto
{

RuleProject::RuleProject(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json RuleProject::toJson() const
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
    // Идентификатор проекта
    if (projectId.has_value())
    {
        result["projectId"] = projectId.value();
    }
    // Право на просмотр элементов (items) проекта
    if (isReader.has_value())
    {
        result["isReader"] = isReader.value();
    }
    // Право на создание и изменение элементов (items) проекта
    if (isWriter.has_value())
    {
        result["isWriter"] = isWriter.value();
    }
    // Право на создание и изменение подпроектов
    if (isProjectEditor.has_value())
    {
        result["isProjectEditor"] = isProjectEditor.value();
    }
    // Право на создание и изменение фаз
    if (isPhaseEditor.has_value())
    {
        result["isPhaseEditor"] = isPhaseEditor.value();
    }
    // Право на создание и изменение досок
    if (isBoardEditor.has_value())
    {
        result["isBoardEditor"] = isBoardEditor.value();
    }

    return result;
}

bool RuleProject::fromJson(const nlohmann::json& json)
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
    // Право на просмотр элементов (items) проекта
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
    // Право на создание и изменение элементов (items) проекта
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
    // Право на создание и изменение подпроектов
    if (json.contains("isProjectEditor") && !json["isProjectEditor"].is_null())
    {
        try
        {
            isProjectEditor = json["isProjectEditor"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isProjectEditor = std::nullopt;
    }
    // Право на создание и изменение фаз
    if (json.contains("isPhaseEditor") && !json["isPhaseEditor"].is_null())
    {
        try
        {
            isPhaseEditor = json["isPhaseEditor"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isPhaseEditor = std::nullopt;
    }
    // Право на создание и изменение досок
    if (json.contains("isBoardEditor") && !json["isBoardEditor"].is_null())
    {
        try
        {
            isBoardEditor = json["isBoardEditor"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isBoardEditor = std::nullopt;
    }

    return success;
}

bool RuleProject::isValid() const
{
    if (!ruleId.has_value())
    {
        return false;
    }
    if (!projectId.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string RuleProject::validationError() const
{
    if (!ruleId.has_value())
    {
        return "Поле «ruleId» является обязательным для заполнения";
    }
    if (!projectId.has_value())
    {
        return "Поле «projectId» является обязательным для заполнения";
    }


    return "";
}

bool RuleProject::operator==(const RuleProject& other) const
{
    return
        id == other.id
        && ruleId == other.ruleId
        && projectId == other.projectId
        && isReader == other.isReader
        && isWriter == other.isWriter
        && isProjectEditor == other.isProjectEditor
        && isPhaseEditor == other.isPhaseEditor
        && isBoardEditor == other.isBoardEditor
;
}

} // namespace dto