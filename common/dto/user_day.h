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
 * @brief UserDay DTO
 */
class UserDay final
{
public:
    using Ptr = std::shared_ptr<UserDay>;
    using ConstPtr = std::shared_ptr<const UserDay>;

public:
    UserDay() = default;
    explicit UserDay(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const UserDay& other) const;
    bool operator!=(const UserDay& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const UserDay& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор пользователя
    std::optional<int64_t> userId;

    /// Дата (уникальна в паре с userId)
    std::optional<std::chrono::system_clock::time_point> date;

    /// Является ли день рабочим для пользователя
    std::optional<bool> isWorkDay;

    /// Время начала работы (если рабочий)
    std::optional<std::string> beginWorkTime;

    /// Время окончания работы (если рабочий)
    std::optional<std::string> endWorkTime;

    /// Длительность перерыва в минутах
    std::optional<int64_t> breakDuration;

    /// Причина (отпуск
    std::optional<std::string> description;

};

inline std::ostream& operator<<(std::ostream& os, const UserDay& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto