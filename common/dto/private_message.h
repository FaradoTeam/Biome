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
 * @brief PrivateMessage DTO
 */
class PrivateMessage final
{
public:
    using Ptr = std::shared_ptr<PrivateMessage>;
    using ConstPtr = std::shared_ptr<const PrivateMessage>;

public:
    PrivateMessage() = default;
    explicit PrivateMessage(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const PrivateMessage& other) const;
    bool operator!=(const PrivateMessage& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const PrivateMessage& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор отправителя
    std::optional<int64_t> senderUserId;

    /// Идентификатор получателя
    std::optional<int64_t> receiverUserId;

    /// Время отправки
    std::optional<std::chrono::system_clock::time_point> creationTimestamp;

    /// Текст сообщения
    std::optional<std::string> content;

    /// Флаг прочтения
    std::optional<bool> isViewed;

};

inline std::ostream& operator<<(std::ostream& os, const PrivateMessage& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto