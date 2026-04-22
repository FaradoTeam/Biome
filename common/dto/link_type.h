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
 * @brief LinkType DTO
 */
class LinkType final
{
public:
    using Ptr = std::shared_ptr<LinkType>;
    using ConstPtr = std::shared_ptr<const LinkType>;

public:
    LinkType() = default;
    explicit LinkType(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const LinkType& other) const;
    bool operator!=(const LinkType& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const LinkType& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Исходный тип элемента
    std::optional<int64_t> sourceItemTypeId;

    /// Целевой тип элемента
    std::optional<int64_t> destinationItemTypeId;

    /// Флаг двунаправленной связи
    std::optional<bool> isBidirectional;

    /// Название типа связи
    std::optional<std::string> caption;

};

inline std::ostream& operator<<(std::ostream& os, const LinkType& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto