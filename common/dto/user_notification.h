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
 * @brief UserNotification DTO
 */
class UserNotification final
{
public:
    using Ptr = std::shared_ptr<UserNotification>;
    using ConstPtr = std::shared_ptr<const UserNotification>;

public:
    UserNotification() = default;
    explicit UserNotification(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const UserNotification& other) const;
    bool operator!=(const UserNotification& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const UserNotification& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор пользователя
    std::optional<int64_t> userId;

    /// Идентификатор элемента
    std::optional<int64_t> itemId;

};

inline std::ostream& operator<<(std::ostream& os, const UserNotification& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto