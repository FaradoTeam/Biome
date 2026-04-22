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
 * @brief ItemLink DTO
 */
class ItemLink final
{
public:
    using Ptr = std::shared_ptr<ItemLink>;
    using ConstPtr = std::shared_ptr<const ItemLink>;

public:
    ItemLink() = default;
    explicit ItemLink(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const ItemLink& other) const;
    bool operator!=(const ItemLink& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const ItemLink& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор типа связи
    std::optional<int64_t> linkTypeId;

    /// Исходный элемент
    std::optional<int64_t> sourceItemId;

    /// Целевой элемент
    std::optional<int64_t> destinationItemId;

};

inline std::ostream& operator<<(std::ostream& os, const ItemLink& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto