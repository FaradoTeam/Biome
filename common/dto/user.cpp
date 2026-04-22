#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

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
        result["first_name"] = firstName.value();
    }
    // Отчество пользователя
    if (middleName.has_value())
    {
        result["middle_name"] = middleName.value();
    }
    // Фамилия пользователя
    if (lastName.has_value())
    {
        result["last_name"] = lastName.value();
    }
    // Email пользователя (уникальный)
    if (email.has_value())
    {
        result["email"] = email.value();
    }
    // Флаг необходимости смены пароля при следующем входе
    if (needChangePassword.has_value())
    {
        result["need_change_password"] = needChangePassword.value();
    }
    // Флаг блокировки пользователя (запрет входа)
    if (isBlocked.has_value())
    {
        result["is_blocked"] = isBlocked.value();
    }
    // Флаг супер-администратора (имеет все права)
    if (isSuperAdmin.has_value())
    {
        result["is_super_admin"] = isSuperAdmin.value();
    }
    // Флаг скрытого пользователя (не отображается в списках)
    if (isHidden.has_value())
    {
        result["is_hidden"] = isHidden.value();
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
    if (json.contains("first_name") && !json["first_name"].is_null())
    {
        try
        {
            firstName = json["first_name"].get<std::string>();
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
    if (json.contains("middle_name") && !json["middle_name"].is_null())
    {
        try
        {
            middleName = json["middle_name"].get<std::string>();
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
    if (json.contains("last_name") && !json["last_name"].is_null())
    {
        try
        {
            lastName = json["last_name"].get<std::string>();
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
    if (json.contains("need_change_password") && !json["need_change_password"].is_null())
    {
        try
        {
            needChangePassword = json["need_change_password"].get<bool>();
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
    if (json.contains("is_blocked") && !json["is_blocked"].is_null())
    {
        try
        {
            isBlocked = json["is_blocked"].get<bool>();
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
    if (json.contains("is_super_admin") && !json["is_super_admin"].is_null())
    {
        try
        {
            isSuperAdmin = json["is_super_admin"].get<bool>();
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
    if (json.contains("is_hidden") && !json["is_hidden"].is_null())
    {
        try
        {
            isHidden = json["is_hidden"].get<bool>();
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