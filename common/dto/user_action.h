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
 * @brief UserAction DTO
 */
class UserAction final
{
public:
    using Ptr = std::shared_ptr<UserAction>;
    using ConstPtr = std::shared_ptr<const UserAction>;

public:
    UserAction() = default;
    explicit UserAction(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const UserAction& other) const;
    bool operator!=(const UserAction& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const UserAction& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор пользователя
    std::optional<int64_t> userId;

    /// Время действия
    std::optional<std::chrono::system_clock::time_point> timestamp;

    /// Краткое описание действия
    std::optional<std::string> caption;

    /// Подробное описание (контекст)
    std::optional<std::string> description;

};

inline std::ostream& operator<<(std::ostream& os, const UserAction& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto