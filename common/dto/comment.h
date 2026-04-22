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
 * @brief Comment DTO
 */
class Comment final
{
public:
    using Ptr = std::shared_ptr<Comment>;
    using ConstPtr = std::shared_ptr<const Comment>;

public:
    Comment() = default;
    explicit Comment(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const Comment& other) const;
    bool operator!=(const Comment& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const Comment& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор автора
    std::optional<int64_t> userId;

    /// Идентификатор элемента
    std::optional<int64_t> itemId;

    /// Время создания
    std::optional<std::chrono::system_clock::time_point> createdAt;

    /// Текст комментария
    std::optional<std::string> content;

};

inline std::ostream& operator<<(std::ostream& os, const Comment& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto