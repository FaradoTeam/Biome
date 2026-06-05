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
 * @brief PlanItem DTO
 */
class PlanItem final
{
public:
    using Ptr = std::shared_ptr<PlanItem>;
    using ConstPtr = std::shared_ptr<const PlanItem>;

public:
    PlanItem() = default;
    explicit PlanItem(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const PlanItem& other) const;
    bool operator!=(const PlanItem& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const PlanItem& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор элемента
    std::optional<int64_t> itemId;

    /// Идентификатор пользователя (null для родительских задач)
    std::optional<int64_t> userId;

    /// Идентификатор экземпляра плана
    std::optional<int64_t> planId;

    /// Плановая дата начала
    std::optional<std::chrono::system_clock::time_point> startDate;

    /// Плановая дата окончания
    std::optional<std::chrono::system_clock::time_point> endDate;

};

inline std::ostream& operator<<(std::ostream& os, const PlanItem& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto