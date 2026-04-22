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
 * @brief StandardDay DTO
 */
class StandardDay final
{
public:
    using Ptr = std::shared_ptr<StandardDay>;
    using ConstPtr = std::shared_ptr<const StandardDay>;

public:
    StandardDay() = default;
    explicit StandardDay(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const StandardDay& other) const;
    bool operator!=(const StandardDay& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const StandardDay& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Номер дня недели (0-6)
    std::optional<int64_t> weekDayNumber;

    /// Порядковый номер недели (для сменного графика)
    std::optional<int64_t> weekOrder;

    /// Является ли день рабочим
    std::optional<bool> isWorkDay;

    /// Время начала работы (HH:MM)
    std::optional<std::string> beginWorkTime;

    /// Время окончания работы (HH:MM)
    std::optional<std::string> endWorkTime;

    /// Длительность перерыва в минутах
    std::optional<int64_t> breakDuration;

};

inline std::ostream& operator<<(std::ostream& os, const StandardDay& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto