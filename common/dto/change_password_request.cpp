#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

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
        result["oldPassword"] = oldPassword.value();
    }
    // newPassword
    if (newPassword.has_value())
    {
        result["newPassword"] = newPassword.value();
    }

    return result;
}

bool ChangePasswordRequest::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // oldPassword
    if (json.contains("oldPassword") && !json["oldPassword"].is_null())
    {
        try
        {
            oldPassword = json["oldPassword"].get<std::string>();
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
    if (json.contains("newPassword") && !json["newPassword"].is_null())
    {
        try
        {
            newPassword = json["newPassword"].get<std::string>();
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
        return "Поле «oldPassword» является обязательным для заполнения";
    }
    if (!newPassword.has_value())
    {
        return "Поле «newPassword» является обязательным для заполнения";
    }

    if (oldPassword.value().empty())
    {
        return "Поле «oldPassword» не может быть пустой строкой";
    }
    if (newPassword.value().empty())
    {
        return "Поле «newPassword» не может быть пустой строкой";
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