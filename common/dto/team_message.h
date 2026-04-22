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
 * @brief TeamMessage DTO
 */
class TeamMessage final
{
public:
    using Ptr = std::shared_ptr<TeamMessage>;
    using ConstPtr = std::shared_ptr<const TeamMessage>;

public:
    TeamMessage() = default;
    explicit TeamMessage(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const TeamMessage& other) const;
    bool operator!=(const TeamMessage& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const TeamMessage& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор отправителя
    std::optional<int64_t> senderUserId;

    /// Идентификатор команды
    std::optional<int64_t> teamId;

    /// Время отправки
    std::optional<std::chrono::system_clock::time_point> creationTimestamp;

    /// Текст сообщения
    std::optional<std::string> content;

};

inline std::ostream& operator<<(std::ostream& os, const TeamMessage& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto