#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "project.h"

namespace dto
{

Project::Project(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json Project::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор проекта
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор родительского проекта (null для корневых)
    if (parentId.has_value())
    {
        result["parent_id"] = parentId.value();
    }
    // Название проекта
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Описание проекта
    if (description.has_value())
    {
        result["description"] = description.value();
    }
    // Дата и время создания
    if (createdAt.has_value())
    {
        result["created_at"] = timePointToSeconds(createdAt.value());
    }
    // Дата и время последнего обновления
    if (updatedAt.has_value())
    {
        result["updated_at"] = timePointToSeconds(updatedAt.value());
    }
    // Флаг архивации (мягкое удаление)
    if (isArchive.has_value())
    {
        result["is_archive"] = isArchive.value();
    }

    return result;
}

bool Project::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // Уникальный идентификатор проекта
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
    // Идентификатор родительского проекта (null для корневых)
    if (json.contains("parent_id") && !json["parent_id"].is_null())
    {
        try
        {
            parentId = json["parent_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        parentId = std::nullopt;
    }
    // Название проекта
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
    // Описание проекта
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
    // Дата и время создания
    if (json.contains("created_at") && !json["created_at"].is_null())
    {
        try
        {
            auto timestampValue = json["created_at"].get<int64_t>();
            createdAt = secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        createdAt = std::nullopt;
    }
    // Дата и время последнего обновления
    if (json.contains("updated_at") && !json["updated_at"].is_null())
    {
        try
        {
            auto timestampValue = json["updated_at"].get<int64_t>();
            updatedAt = secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        updatedAt = std::nullopt;
    }
    // Флаг архивации (мягкое удаление)
    if (json.contains("is_archive") && !json["is_archive"].is_null())
    {
        try
        {
            isArchive = json["is_archive"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isArchive = std::nullopt;
    }

    return success;
}

bool Project::isValid() const
{
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

std::string Project::validationError() const
{
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

bool Project::operator==(const Project& other) const
{
    return
        id == other.id
        && parentId == other.parentId
        && caption == other.caption
        && description == other.description
        && createdAt == other.createdAt
        && updatedAt == other.updatedAt
        && isArchive == other.isArchive
;
}

} // namespace dto