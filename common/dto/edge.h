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
 * @brief Edge DTO
 */
class Edge final
{
public:
    using Ptr = std::shared_ptr<Edge>;
    using ConstPtr = std::shared_ptr<const Edge>;

public:
    Edge() = default;
    explicit Edge(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const Edge& other) const;
    bool operator!=(const Edge& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const Edge& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Начальное состояние
    std::optional<int64_t> beginStateId;

    /// Конечное состояние
    std::optional<int64_t> endStateId;

};

inline std::ostream& operator<<(std::ostream& os, const Edge& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto