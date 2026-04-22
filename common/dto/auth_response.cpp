#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "auth_response.h"

namespace dto
{

AuthResponse::AuthResponse(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json AuthResponse::toJson() const
{
    nlohmann::json result;

    // accessToken
    if (accessToken.has_value())
    {
        result["access_token"] = accessToken.value();
    }
    // tokenType
    result["token_type"] = tokenType;

    return result;
}

bool AuthResponse::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // accessToken
    if (json.contains("access_token") && !json["access_token"].is_null())
    {
        try
        {
            accessToken = json["access_token"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        accessToken = std::nullopt;
    }
    // tokenType
    // Constant field - ignore value from JSON

    return success;
}

bool AuthResponse::isValid() const
{

    // Дополнительные проверки для непустых значений

    return true;
}

std::string AuthResponse::validationError() const
{


    return "";
}

bool AuthResponse::operator==(const AuthResponse& other) const
{
    return
        accessToken == other.accessToken
;
}

} // namespace dto