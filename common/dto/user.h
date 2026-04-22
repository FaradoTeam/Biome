#pragma once

#include <optional>
#include <chrono>
#include <ctime>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "common/helpers/print_helpers.h"


namespace dto
{

/**
 * @brief User DTO
 */
class User final
{
public:
    using Ptr = std::shared_ptr<User>;
    using ConstPtr = std::shared_ptr<const User>;

public:
    User() = default;
    explicit User(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const User& other) const;
    bool operator!=(const User& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const User& dto);

public:
    /// Уникальный идентификатор пользователя
    std::optional<int64_t> id;

    /// Логин пользователя (уникальный)
    std::optional<std::string> login;

    /// Имя пользователя
    std::optional<std::string> firstName;

    /// Отчество пользователя
    std::optional<std::string> middleName;

    /// Фамилия пользователя
    std::optional<std::string> lastName;

    /// Email пользователя (уникальный)
    std::optional<std::string> email;

    /// Флаг необходимости смены пароля при следующем входе
    std::optional<bool> needChangePassword;

    /// Флаг блокировки пользователя (запрет входа)
    std::optional<bool> isBlocked;

    /// Флаг супер-администратора (имеет все права)
    std::optional<bool> isSuperAdmin;

    /// Флаг скрытого пользователя (не отображается в списках)
    std::optional<bool> isHidden;

};

inline std::ostream& operator<<(std::ostream& os, const User& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto