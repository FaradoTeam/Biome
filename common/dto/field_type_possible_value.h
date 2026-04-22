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
 * @brief FieldTypePossibleValue DTO
 */
class FieldTypePossibleValue final
{
public:
    using Ptr = std::shared_ptr<FieldTypePossibleValue>;
    using ConstPtr = std::shared_ptr<const FieldTypePossibleValue>;

public:
    FieldTypePossibleValue() = default;
    explicit FieldTypePossibleValue(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const FieldTypePossibleValue& other) const;
    bool operator!=(const FieldTypePossibleValue& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const FieldTypePossibleValue& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор типа поля
    std::optional<int64_t> fieldTypeId;

    /// Значение для выбора
    std::optional<std::string> value;

};

inline std::ostream& operator<<(std::ostream& os, const FieldTypePossibleValue& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto