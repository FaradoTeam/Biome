#pragma once

#include <optional>
#include <string>

namespace terminal::models
{

/**
 * @brief Клиентская модель пользователя.
 *
 * Зеркалирует dto::User на сервере, но без привязки к nlohmann/json.
 */
struct User final
{
    std::optional<int64_t> id;
    std::optional<std::string> login;
    std::optional<std::string> firstName;
    std::optional<std::string> middleName;
    std::optional<std::string> lastName;
    std::optional<std::string> email;
    std::optional<bool> needChangePassword;
    std::optional<bool> isBlocked;
    std::optional<bool> isSuperAdmin;
    std::optional<bool> isHidden;
};

} // namespace terminal::models
