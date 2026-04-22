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
 * @brief Phase DTO
 */
class Phase final
{
public:
    using Ptr = std::shared_ptr<Phase>;
    using ConstPtr = std::shared_ptr<const Phase>;

public:
    Phase() = default;
    explicit Phase(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const Phase& other) const;
    bool operator!=(const Phase& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const Phase& dto);

public:
    /// Уникальный идентификатор фазы
    std::optional<int64_t> id;

    /// Идентификатор проекта
    std::optional<int64_t> projectId;

    /// Название фазы
    std::optional<std::string> caption;

    /// Описание фазы
    std::optional<std::string> description;

    /// Дата начала фазы
    std::optional<std::chrono::system_clock::time_point> beginDate;

    /// Дата окончания фазы
    std::optional<std::chrono::system_clock::time_point> endDate;

    /// Флаг архивации фазы
    std::optional<bool> isArchive;

};

inline std::ostream& operator<<(std::ostream& os, const Phase& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto