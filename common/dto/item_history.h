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
 * @brief ItemHistory DTO
 */
class ItemHistory final
{
public:
    using Ptr = std::shared_ptr<ItemHistory>;
    using ConstPtr = std::shared_ptr<const ItemHistory>;

public:
    ItemHistory() = default;
    explicit ItemHistory(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const ItemHistory& other) const;
    bool operator!=(const ItemHistory& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const ItemHistory& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор элемента
    std::optional<int64_t> itemId;

    /// Идентификатор пользователя
    std::optional<int64_t> userId;

    /// Время изменения
    std::optional<std::chrono::system_clock::time_point> timestamp;

    /// JSON с изменениями (diff)
    std::optional<std::string> diff;

};

inline std::ostream& operator<<(std::ostream& os, const ItemHistory& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto