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
 * @brief ItemField DTO
 */
class ItemField final
{
public:
    using Ptr = std::shared_ptr<ItemField>;
    using ConstPtr = std::shared_ptr<const ItemField>;

public:
    ItemField() = default;
    explicit ItemField(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const ItemField& other) const;
    bool operator!=(const ItemField& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const ItemField& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор элемента
    std::optional<int64_t> itemId;

    /// Идентификатор типа поля
    std::optional<int64_t> fieldTypeId;

    /// Значение поля (в сериализованном виде)
    std::optional<std::string> value;

};

inline std::ostream& operator<<(std::ostream& os, const ItemField& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto