#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

#include "fork_plan_request.h"

namespace dto
{

ForkPlanRequest::ForkPlanRequest(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json ForkPlanRequest::toJson() const
{
    nlohmann::json result;

    // Название нового плана
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Описание нового плана
    if (description.has_value())
    {
        result["description"] = description.value();
    }

    return result;
}

bool ForkPlanRequest::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // Название нового плана
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
    // Описание нового плана
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

bool ForkPlanRequest::isValid() const
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

std::string ForkPlanRequest::validationError() const
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

bool ForkPlanRequest::operator==(const ForkPlanRequest& other) const
{
    return
        caption == other.caption
        && description == other.description
;
}

} // namespace dto