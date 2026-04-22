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
 * @brief ItemType DTO
 */
class ItemType final
{
public:
    using Ptr = std::shared_ptr<ItemType>;
    using ConstPtr = std::shared_ptr<const ItemType>;

public:
    ItemType() = default;
    explicit ItemType(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const ItemType& other) const;
    bool operator!=(const ItemType& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const ItemType& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор рабочего процесса
    std::optional<int64_t> workflowId;

    /// Состояние по умолчанию для новых элементов
    std::optional<int64_t> defaultStateId;

    /// Название типа
    std::optional<std::string> caption;

    /// Вид элемента
    std::optional<std::string> kind;

    /// Содержимое по умолчанию (шаблон для создания новых item)
    std::optional<std::string> defaultContent;

};

inline std::ostream& operator<<(std::ostream& os, const ItemType& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto