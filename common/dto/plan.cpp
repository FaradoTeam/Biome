#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "plan.h"

namespace dto
{

Plan::Plan(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json Plan::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор фазы
    if (phaseId.has_value())
    {
        result["phaseId"] = phaseId.value();
    }
    // Идентификатор плана
    if (basePlanId.has_value())
    {
        result["basePlanId"] = basePlanId.value();
    }
    // Название плана
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Описание плана
    if (description.has_value())
    {
        result["description"] = description.value();
    }
    // Флаг
    if (isActive.has_value())
    {
        result["isActive"] = isActive.value();
    }
    // Дата и время создания
    if (createdAt.has_value())
    {
        result["createdAt"] = timePointToSeconds(createdAt.value());
    }
    // Идентификатор пользователя
    if (createdByUserId.has_value())
    {
        result["createdByUserId"] = createdByUserId.value();
    }
    // Дата и время активации
    if (activatedAt.has_value())
    {
        result["activatedAt"] = timePointToSeconds(activatedAt.value());
    }
    // Идентификатор пользователя
    if (activatedByUserId.has_value())
    {
        result["activatedByUserId"] = activatedByUserId.value();
    }

    return result;
}

bool Plan::fromJson(const nlohmann::json& json)
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
    // Идентификатор фазы
    if (json.contains("phaseId") && !json["phaseId"].is_null())
    {
        try
        {
            phaseId = json["phaseId"].get<int64_t>();
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
    // Идентификатор плана
    if (json.contains("basePlanId") && !json["basePlanId"].is_null())
    {
        try
        {
            basePlanId = json["basePlanId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        basePlanId = std::nullopt;
    }
    // Название плана
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
    // Описание плана
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
    // Флаг
    if (json.contains("isActive") && !json["isActive"].is_null())
    {
        try
        {
            isActive = json["isActive"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isActive = std::nullopt;
    }
    // Дата и время создания
    if (json.contains("createdAt") && !json["createdAt"].is_null())
    {
        try
        {
            auto timestampValue = json["createdAt"].get<int64_t>();
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
    // Идентификатор пользователя
    if (json.contains("createdByUserId") && !json["createdByUserId"].is_null())
    {
        try
        {
            createdByUserId = json["createdByUserId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        createdByUserId = std::nullopt;
    }
    // Дата и время активации
    if (json.contains("activatedAt") && !json["activatedAt"].is_null())
    {
        try
        {
            auto timestampValue = json["activatedAt"].get<int64_t>();
            activatedAt = secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        activatedAt = std::nullopt;
    }
    // Идентификатор пользователя
    if (json.contains("activatedByUserId") && !json["activatedByUserId"].is_null())
    {
        try
        {
            activatedByUserId = json["activatedByUserId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        activatedByUserId = std::nullopt;
    }

    return success;
}

bool Plan::isValid() const
{
    if (!phaseId.has_value())
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

std::string Plan::validationError() const
{
    if (!phaseId.has_value())
    {
        return "Поле «phaseId» является обязательным для заполнения";
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

bool Plan::operator==(const Plan& other) const
{
    return
        id == other.id
        && phaseId == other.phaseId
        && basePlanId == other.basePlanId
        && caption == other.caption
        && description == other.description
        && isActive == other.isActive
        && createdAt == other.createdAt
        && createdByUserId == other.createdByUserId
        && activatedAt == other.activatedAt
        && activatedByUserId == other.activatedByUserId
;
}

} // namespace dto