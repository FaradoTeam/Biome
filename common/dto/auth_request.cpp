#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "auth_request.h"

namespace dto
{

AuthRequest::AuthRequest(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json AuthRequest::toJson() const
{
    nlohmann::json result;

    // login
    if (login.has_value())
    {
        result["login"] = login.value();
    }
    // password
    if (password.has_value())
    {
        result["password"] = password.value();
    }

    return result;
}

bool AuthRequest::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // login
    if (json.contains("login") && !json["login"].is_null())
    {
        try
        {
            login = json["login"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        login = std::nullopt;
    }
    // password
    if (json.contains("password") && !json["password"].is_null())
    {
        try
        {
            password = json["password"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        password = std::nullopt;
    }

    return success;
}

bool AuthRequest::isValid() const
{
    if (!login.has_value())
    {
        return false;
    }
    if (!password.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений
    if (login.value().empty())
    {
        return false;
    }
    if (password.value().empty())
    {
        return false;
    }

    return true;
}

std::string AuthRequest::validationError() const
{
    if (!login.has_value())
    {
        return "Поле «login» является обязательным для заполнения";
    }
    if (!password.has_value())
    {
        return "Поле «password» является обязательным для заполнения";
    }

    if (login.value().empty())
    {
        return "Поле «login» не может быть пустой строкой";
    }
    if (password.value().empty())
    {
        return "Поле «password» не может быть пустой строкой";
    }

    return "";
}

bool AuthRequest::operator==(const AuthRequest& other) const
{
    return
        login == other.login
        && password == other.password
;
}

} // namespace dto