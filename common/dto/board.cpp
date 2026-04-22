#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "board.h"

namespace dto
{

Board::Board(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json Board::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор рабочего процесса
    if (workflowId.has_value())
    {
        result["workflow_id"] = workflowId.value();
    }
    // Идентификатор проекта
    if (projectId.has_value())
    {
        result["project_id"] = projectId.value();
    }
    // Идентификатор фазы
    if (phaseId.has_value())
    {
        result["phase_id"] = phaseId.value();
    }
    // Название доски
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Описание доски
    if (description.has_value())
    {
        result["description"] = description.value();
    }

    return result;
}

bool Board::fromJson(const nlohmann::json& json)
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
    // Идентификатор рабочего процесса
    if (json.contains("workflow_id") && !json["workflow_id"].is_null())
    {
        try
        {
            workflowId = json["workflow_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        workflowId = std::nullopt;
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
    // Идентификатор фазы
    if (json.contains("phase_id") && !json["phase_id"].is_null())
    {
        try
        {
            phaseId = json["phase_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        phaseId = std::nullopt;
    }
    // Название доски
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
    // Описание доски
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

    return success;
}

bool Board::isValid() const
{
    if (!workflowId.has_value())
    {
        return false;
    }
    if (!caption.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений
    if (caption.value().empty())
    {
        return false;
    }

    return true;
}

std::string Board::validationError() const
{
    if (!workflowId.has_value())
    {
        return "Поле «workflow_id» является обязательным для заполнения";
    }
    if (!caption.has_value())
    {
        return "Поле «caption» является обязательным для заполнения";
    }

    if (caption.value().empty())
    {
        return "Поле «caption» не может быть пустой строкой";
    }

    return "";
}

bool Board::operator==(const Board& other) const
{
    return
        id == other.id
        && workflowId == other.workflowId
        && projectId == other.projectId
        && phaseId == other.phaseId
        && caption == other.caption
        && description == other.description
;
}

} // namespace dto