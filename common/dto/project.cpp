#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

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
        result["parentId"] = parentId.value();
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
        result["createdAt"] = common::timePointToSeconds(createdAt.value());
    }
    // Дата и время последнего обновления
    if (updatedAt.has_value())
    {
        result["updatedAt"] = common::timePointToSeconds(updatedAt.value());
    }
    // Флаг архивации (мягкое удаление)
    if (isArchive.has_value())
    {
        result["isArchive"] = isArchive.value();
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
    if (json.contains("parentId") && !json["parentId"].is_null())
    {
        try
        {
            parentId = json["parentId"].get<int64_t>();
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
    if (json.contains("createdAt") && !json["createdAt"].is_null())
    {
        try
        {
            auto timestampValue = json["createdAt"].get<int64_t>();
            createdAt = common::secondsToTimePoint(timestampValue);
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
    if (json.contains("updatedAt") && !json["updatedAt"].is_null())
    {
        try
        {
            auto timestampValue = json["updatedAt"].get<int64_t>();
            updatedAt = common::secondsToTimePoint(timestampValue);
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
    if (json.contains("isArchive") && !json["isArchive"].is_null())
    {
        try
        {
            isArchive = json["isArchive"].get<bool>();
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