#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

#include "activate_plan_request.h"

namespace dto
{

ActivatePlanRequest::ActivatePlanRequest(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json ActivatePlanRequest::toJson() const
{
    nlohmann::json result;

    // Кто активирует план
    if (activatedByUserId.has_value())
    {
        result["activatedByUserId"] = activatedByUserId.value();
    }

    return result;
}

bool ActivatePlanRequest::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // Кто активирует план
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

bool ActivatePlanRequest::isValid() const
{
    if (!activatedByUserId.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string ActivatePlanRequest::validationError() const
{
    if (!activatedByUserId.has_value())
    {
        return "Поле «activatedByUserId» является обязательным для заполнения";
    }


    return "";
}

bool ActivatePlanRequest::operator==(const ActivatePlanRequest& other) const
{
    return
        activatedByUserId == other.activatedByUserId
;
}

} // namespace dto