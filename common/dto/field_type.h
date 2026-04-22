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
 * @brief FieldType DTO
 */
class FieldType final
{
public:
    using Ptr = std::shared_ptr<FieldType>;
    using ConstPtr = std::shared_ptr<const FieldType>;

public:
    FieldType() = default;
    explicit FieldType(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const FieldType& other) const;
    bool operator!=(const FieldType& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const FieldType& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор типа элемента
    std::optional<int64_t> itemTypeId;

    /// Название поля
    std::optional<std::string> caption;

    /// Описание поля
    std::optional<std::string> description;

    /// Тип значения
    std::optional<std::string> valueType;

    /// Отображать поле на канбан-доске
    std::optional<bool> isBoardVisible;

};

inline std::ostream& operator<<(std::ostream& os, const FieldType& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto