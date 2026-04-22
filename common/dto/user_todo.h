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
 * @brief UserTodo DTO
 */
class UserTodo final
{
public:
    using Ptr = std::shared_ptr<UserTodo>;
    using ConstPtr = std::shared_ptr<const UserTodo>;

public:
    UserTodo() = default;
    explicit UserTodo(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const UserTodo& other) const;
    bool operator!=(const UserTodo& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const UserTodo& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор элемента
    std::optional<int64_t> itemId;

    /// Идентификатор пользователя
    std::optional<int64_t> userId;

    /// Плановая дата начала
    std::optional<std::chrono::system_clock::time_point> startDate;

    /// Плановая дата окончания
    std::optional<std::chrono::system_clock::time_point> endDate;

};

inline std::ostream& operator<<(std::ostream& os, const UserTodo& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto