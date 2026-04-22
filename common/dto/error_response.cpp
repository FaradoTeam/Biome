#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "error_response.h"

namespace dto
{

ErrorResponse::ErrorResponse(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json ErrorResponse::toJson() const
{
    nlohmann::json result;

    // Код ошибки HTTP
    if (code.has_value())
    {
        result["code"] = code.value();
    }
    // Сообщение об ошибке
    if (message.has_value())
    {
        result["message"] = message.value();
    }
    // Дополнительные детали ошибки
    if (details.has_value())
    {
        result["details"] = details.value();
    }

    return result;
}

bool ErrorResponse::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // Код ошибки HTTP
    if (json.contains("code") && !json["code"].is_null())
    {
        try
        {
            code = json["code"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        code = std::nullopt;
    }
    // Сообщение об ошибке
    if (json.contains("message") && !json["message"].is_null())
    {
        try
        {
            message = json["message"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        message = std::nullopt;
    }
    // Дополнительные детали ошибки
    if (json.contains("details") && !json["details"].is_null())
    {
        try
        {
            details = json["details"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        details = std::nullopt;
    }

    return success;
}

bool ErrorResponse::isValid() const
{
    if (!code.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string ErrorResponse::validationError() const
{
    if (!code.has_value())
    {
        return "Поле «code» является обязательным для заполнения";
    }


    return "";
}

bool ErrorResponse::operator==(const ErrorResponse& other) const
{
    return
        code == other.code
        && message == other.message
        && details == other.details
;
}

} // namespace dto