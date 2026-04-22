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
 * @brief Item DTO
 */
class Item final
{
public:
    using Ptr = std::shared_ptr<Item>;
    using ConstPtr = std::shared_ptr<const Item>;

public:
    Item() = default;
    explicit Item(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const Item& other) const;
    bool operator!=(const Item& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const Item& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор типа элемента
    std::optional<int64_t> itemTypeId;

    /// Идентификатор родительского элемента
    std::optional<int64_t> parentId;

    /// Идентификатор текущего состояния
    std::optional<int64_t> stateId;

    /// Идентификатор фазы
    std::optional<int64_t> phaseId;

    /// Название элемента
    std::optional<std::string> caption;

    /// Содержимое элемента
    std::optional<std::string> content;

    /// Флаг удаления (мягкое удаление)
    std::optional<bool> isDeleted;

};

inline std::ostream& operator<<(std::ostream& os, const Item& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto