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
 * @brief ItemUserState DTO
 */
class ItemUserState final
{
public:
    using Ptr = std::shared_ptr<ItemUserState>;
    using ConstPtr = std::shared_ptr<const ItemUserState>;

public:
    ItemUserState() = default;
    explicit ItemUserState(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const ItemUserState& other) const;
    bool operator!=(const ItemUserState& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const ItemUserState& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор элемента
    std::optional<int64_t> itemId;

    /// Идентификатор пользователя
    std::optional<int64_t> userId;

    /// Идентификатор состояния
    std::optional<int64_t> stateId;

    /// Комментарий к переходу
    std::optional<std::string> comment;

    /// Время перехода
    std::optional<std::chrono::system_clock::time_point> timestamp;

};

inline std::ostream& operator<<(std::ostream& os, const ItemUserState& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto