#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "change_password_request.h"

namespace dto
{

ChangePasswordRequest::ChangePasswordRequest(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json ChangePasswordRequest::toJson() const
{
    nlohmann::json result;

    // oldPassword
    if (oldPassword.has_value())
    {
        result["old_password"] = oldPassword.value();
    }
    // newPassword
    if (newPassword.has_value())
    {
        result["new_password"] = newPassword.value();
    }

    return result;
}

bool ChangePasswordRequest::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // oldPassword
    if (json.contains("old_password") && !json["old_password"].is_null())
    {
        try
        {
            oldPassword = json["old_password"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        oldPassword = std::nullopt;
    }
    // newPassword
    if (json.contains("new_password") && !json["new_password"].is_null())
    {
        try
        {
            newPassword = json["new_password"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        newPassword = std::nullopt;
    }

    return success;
}

bool ChangePasswordRequest::isValid() const
{
    if (!oldPassword.has_value())
    {
        return false;
    }
    if (!newPassword.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений
    if (oldPassword.value().empty())
    {
        return false;
    }
    if (newPassword.value().empty())
    {
        return false;
    }

    return true;
}

std::string ChangePasswordRequest::validationError() const
{
    if (!oldPassword.has_value())
    {
        return "Поле «old_password» является обязательным для заполнения";
    }
    if (!newPassword.has_value())
    {
        return "Поле «new_password» является обязательным для заполнения";
    }

    if (oldPassword.value().empty())
    {
        return "Поле «old_password» не может быть пустой строкой";
    }
    if (newPassword.value().empty())
    {
        return "Поле «new_password» не может быть пустой строкой";
    }

    return "";
}

bool ChangePasswordRequest::operator==(const ChangePasswordRequest& other) const
{
    return
        oldPassword == other.oldPassword
        && newPassword == other.newPassword
;
}

} // namespace dto