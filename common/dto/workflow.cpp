#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "workflow.h"

namespace dto
{

Workflow::Workflow(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json Workflow::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Название рабочего процесса
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Описание рабочего процесса
    if (description.has_value())
    {
        result["description"] = description.value();
    }

    return result;
}

bool Workflow::fromJson(const nlohmann::json& json)
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
    // Название рабочего процесса
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
    // Описание рабочего процесса
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

bool Workflow::isValid() const
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

std::string Workflow::validationError() const
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

bool Workflow::operator==(const Workflow& other) const
{
    return
        id == other.id
        && caption == other.caption
        && description == other.description
;
}

} // namespace dto