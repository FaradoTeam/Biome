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
 * @brief UserTeamRole DTO
 */
class UserTeamRole final
{
public:
    using Ptr = std::shared_ptr<UserTeamRole>;
    using ConstPtr = std::shared_ptr<const UserTeamRole>;

public:
    UserTeamRole() = default;
    explicit UserTeamRole(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const UserTeamRole& other) const;
    bool operator!=(const UserTeamRole& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const UserTeamRole& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор пользователя
    std::optional<int64_t> userId;

    /// Идентификатор команды
    std::optional<int64_t> teamId;

    /// Идентификатор роли
    std::optional<int64_t> roleId;

};

inline std::ostream& operator<<(std::ostream& os, const UserTeamRole& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto