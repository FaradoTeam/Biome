#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "phase.h"

namespace dto
{

Phase::Phase(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json Phase::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор фазы
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор проекта
    if (projectId.has_value())
    {
        result["project_id"] = projectId.value();
    }
    // Название фазы
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Описание фазы
    if (description.has_value())
    {
        result["description"] = description.value();
    }
    // Дата начала фазы
    if (beginDate.has_value())
    {
        result["begin_date"] = timePointToSeconds(beginDate.value());
    }
    // Дата окончания фазы
    if (endDate.has_value())
    {
        result["end_date"] = timePointToSeconds(endDate.value());
    }
    // Флаг архивации фазы
    if (isArchive.has_value())
    {
        result["is_archive"] = isArchive.value();
    }

    return result;
}

bool Phase::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // Уникальный идентификатор фазы
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
    // Название фазы
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
    // Описание фазы
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
    // Дата начала фазы
    if (json.contains("begin_date") && !json["begin_date"].is_null())
    {
        try
        {
            auto timestampValue = json["begin_date"].get<int64_t>();
            beginDate = secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        beginDate = std::nullopt;
    }
    // Дата окончания фазы
    if (json.contains("end_date") && !json["end_date"].is_null())
    {
        try
        {
            auto timestampValue = json["end_date"].get<int64_t>();
            endDate = secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        endDate = std::nullopt;
    }
    // Флаг архивации фазы
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

bool Phase::isValid() const
{
    if (!projectId.has_value())
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

std::string Phase::validationError() const
{
    if (!projectId.has_value())
    {
        return "Поле «project_id» является обязательным для заполнения";
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

bool Phase::operator==(const Phase& other) const
{
    return
        id == other.id
        && projectId == other.projectId
        && caption == other.caption
        && description == other.description
        && beginDate == other.beginDate
        && endDate == other.endDate
        && isArchive == other.isArchive
;
}

} // namespace dto