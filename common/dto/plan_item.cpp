#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

#include "plan_item.h"

namespace dto
{

PlanItem::PlanItem(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json PlanItem::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор элемента
    if (itemId.has_value())
    {
        result["itemId"] = itemId.value();
    }
    // Идентификатор пользователя (null для родительских задач)
    if (userId.has_value())
    {
        result["userId"] = userId.value();
    }
    // Идентификатор экземпляра плана
    if (planId.has_value())
    {
        result["planId"] = planId.value();
    }
    // Плановая дата начала
    if (startDate.has_value())
    {
        result["startDate"] = common::timePointToSeconds(startDate.value());
    }
    // Плановая дата окончания
    if (endDate.has_value())
    {
        result["endDate"] = common::timePointToSeconds(endDate.value());
    }

    return result;
}

bool PlanItem::fromJson(const nlohmann::json& json)
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
    // Идентификатор элемента
    if (json.contains("itemId") && !json["itemId"].is_null())
    {
        try
        {
            itemId = json["itemId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        itemId = std::nullopt;
    }
    // Идентификатор пользователя (null для родительских задач)
    if (json.contains("userId") && !json["userId"].is_null())
    {
        try
        {
            userId = json["userId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        userId = std::nullopt;
    }
    // Идентификатор экземпляра плана
    if (json.contains("planId") && !json["planId"].is_null())
    {
        try
        {
            planId = json["planId"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        planId = std::nullopt;
    }
    // Плановая дата начала
    if (json.contains("startDate") && !json["startDate"].is_null())
    {
        try
        {
            auto timestampValue = json["startDate"].get<int64_t>();
            startDate = common::secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        startDate = std::nullopt;
    }
    // Плановая дата окончания
    if (json.contains("endDate") && !json["endDate"].is_null())
    {
        try
        {
            auto timestampValue = json["endDate"].get<int64_t>();
            endDate = common::secondsToTimePoint(timestampValue);
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

    return success;
}

bool PlanItem::isValid() const
{
    if (!itemId.has_value())
    {
        return false;
    }
    if (!planId.has_value())
    {
        return false;
    }
    if (!startDate.has_value())
    {
        return false;
    }
    if (!endDate.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string PlanItem::validationError() const
{
    if (!itemId.has_value())
    {
        return "Поле «itemId» является обязательным для заполнения";
    }
    if (!planId.has_value())
    {
        return "Поле «planId» является обязательным для заполнения";
    }
    if (!startDate.has_value())
    {
        return "Поле «startDate» является обязательным для заполнения";
    }
    if (!endDate.has_value())
    {
        return "Поле «endDate» является обязательным для заполнения";
    }


    return "";
}

bool PlanItem::operator==(const PlanItem& other) const
{
    return
        id == other.id
        && itemId == other.itemId
        && userId == other.userId
        && planId == other.planId
        && startDate == other.startDate
        && endDate == other.endDate
;
}

} // namespace dto