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
 * @brief Plan DTO
 */
class Plan final
{
public:
    using Ptr = std::shared_ptr<Plan>;
    using ConstPtr = std::shared_ptr<const Plan>;

public:
    Plan() = default;
    explicit Plan(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const Plan& other) const;
    bool operator!=(const Plan& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const Plan& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор фазы
    std::optional<int64_t> phaseId;

    /// Идентификатор плана
    std::optional<int64_t> basePlanId;

    /// Название плана
    std::optional<std::string> caption;

    /// Описание плана
    std::optional<std::string> description;

    /// Флаг
    std::optional<bool> isActive;

    /// Дата и время создания
    std::optional<std::chrono::system_clock::time_point> createdAt;

    /// Идентификатор пользователя
    std::optional<int64_t> createdByUserId;

    /// Дата и время активации
    std::optional<std::chrono::system_clock::time_point> activatedAt;

    /// Идентификатор пользователя
    std::optional<int64_t> activatedByUserId;

};

inline std::ostream& operator<<(std::ostream& os, const Plan& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto