#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

#include "user.h"

namespace dto
{

User::User(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json User::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор пользователя
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Логин пользователя (уникальный)
    if (login.has_value())
    {
        result["login"] = login.value();
    }
    // Имя пользователя
    if (firstName.has_value())
    {
        result["firstName"] = firstName.value();
    }
    // Отчество пользователя
    if (middleName.has_value())
    {
        result["middleName"] = middleName.value();
    }
    // Фамилия пользователя
    if (lastName.has_value())
    {
        result["lastName"] = lastName.value();
    }
    // Email пользователя (уникальный)
    if (email.has_value())
    {
        result["email"] = email.value();
    }
    // Флаг необходимости смены пароля при следующем входе
    if (needChangePassword.has_value())
    {
        result["needChangePassword"] = needChangePassword.value();
    }
    // Флаг блокировки пользователя (запрет входа)
    if (isBlocked.has_value())
    {
        result["isBlocked"] = isBlocked.value();
    }
    // Флаг супер-администратора (имеет все права)
    if (isSuperAdmin.has_value())
    {
        result["isSuperAdmin"] = isSuperAdmin.value();
    }
    // Флаг скрытого пользователя (не отображается в списках)
    if (isHidden.has_value())
    {
        result["isHidden"] = isHidden.value();
    }

    return result;
}

bool User::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // Уникальный идентификатор пользователя
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
    // Логин пользователя (уникальный)
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
    // Имя пользователя
    if (json.contains("firstName") && !json["firstName"].is_null())
    {
        try
        {
            firstName = json["firstName"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        firstName = std::nullopt;
    }
    // Отчество пользователя
    if (json.contains("middleName") && !json["middleName"].is_null())
    {
        try
        {
            middleName = json["middleName"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        middleName = std::nullopt;
    }
    // Фамилия пользователя
    if (json.contains("lastName") && !json["lastName"].is_null())
    {
        try
        {
            lastName = json["lastName"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        lastName = std::nullopt;
    }
    // Email пользователя (уникальный)
    if (json.contains("email") && !json["email"].is_null())
    {
        try
        {
            email = json["email"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        email = std::nullopt;
    }
    // Флаг необходимости смены пароля при следующем входе
    if (json.contains("needChangePassword") && !json["needChangePassword"].is_null())
    {
        try
        {
            needChangePassword = json["needChangePassword"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        needChangePassword = std::nullopt;
    }
    // Флаг блокировки пользователя (запрет входа)
    if (json.contains("isBlocked") && !json["isBlocked"].is_null())
    {
        try
        {
            isBlocked = json["isBlocked"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isBlocked = std::nullopt;
    }
    // Флаг супер-администратора (имеет все права)
    if (json.contains("isSuperAdmin") && !json["isSuperAdmin"].is_null())
    {
        try
        {
            isSuperAdmin = json["isSuperAdmin"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isSuperAdmin = std::nullopt;
    }
    // Флаг скрытого пользователя (не отображается в списках)
    if (json.contains("isHidden") && !json["isHidden"].is_null())
    {
        try
        {
            isHidden = json["isHidden"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isHidden = std::nullopt;
    }

    return success;
}

bool User::isValid() const
{
    if (!login.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений
    if (login.value().empty())
    {
        return false;
    }

    return true;
}

std::string User::validationError() const
{
    if (!login.has_value())
    {
        return "Поле «login» является обязательным для заполнения";
    }

    if (login.value().empty())
    {
        return "Поле «login» не может быть пустой строкой";
    }

    return "";
}

bool User::operator==(const User& other) const
{
    return
        id == other.id
        && login == other.login
        && firstName == other.firstName
        && middleName == other.middleName
        && lastName == other.lastName
        && email == other.email
        && needChangePassword == other.needChangePassword
        && isBlocked == other.isBlocked
        && isSuperAdmin == other.isSuperAdmin
        && isHidden == other.isHidden
;
}

} // namespace dto