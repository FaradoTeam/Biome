#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

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
        result["rule_id"] = ruleId.value();
    }
    // Идентификатор проекта
    if (projectId.has_value())
    {
        result["project_id"] = projectId.value();
    }
    // Право на просмотр элементов (items) проекта
    if (isReader.has_value())
    {
        result["is_reader"] = isReader.value();
    }
    // Право на создание и изменение элементов (items) проекта
    if (isWriter.has_value())
    {
        result["is_writer"] = isWriter.value();
    }
    // Право на создание и изменение подпроектов
    if (isProjectEditor.has_value())
    {
        result["is_project_editor"] = isProjectEditor.value();
    }
    // Право на создание и изменение фаз
    if (isPhaseEditor.has_value())
    {
        result["is_phase_editor"] = isPhaseEditor.value();
    }
    // Право на создание и изменение досок
    if (isBoardEditor.has_value())
    {
        result["is_board_editor"] = isBoardEditor.value();
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
    // Идентификатор проекта
    if (json.contains("project_id") && !json["project_id"].is_null())
    {
        try
        {
            projectId = json["project_id"].get<int64_t>();
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
    if (json.contains("is_reader") && !json["is_reader"].is_null())
    {
        try
        {
            isReader = json["is_reader"].get<bool>();
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
    if (json.contains("is_writer") && !json["is_writer"].is_null())
    {
        try
        {
            isWriter = json["is_writer"].get<bool>();
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
    if (json.contains("is_project_editor") && !json["is_project_editor"].is_null())
    {
        try
        {
            isProjectEditor = json["is_project_editor"].get<bool>();
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
    if (json.contains("is_phase_editor") && !json["is_phase_editor"].is_null())
    {
        try
        {
            isPhaseEditor = json["is_phase_editor"].get<bool>();
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
    if (json.contains("is_board_editor") && !json["is_board_editor"].is_null())
    {
        try
        {
            isBoardEditor = json["is_board_editor"].get<bool>();
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
        return "Поле «rule_id» является обязательным для заполнения";
    }
    if (!projectId.has_value())
    {
        return "Поле «project_id» является обязательным для заполнения";
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