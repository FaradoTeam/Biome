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
 * @brief ErrorResponse DTO
 */
class ErrorResponse final
{
public:
    using Ptr = std::shared_ptr<ErrorResponse>;
    using ConstPtr = std::shared_ptr<const ErrorResponse>;

public:
    ErrorResponse() = default;
    explicit ErrorResponse(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const ErrorResponse& other) const;
    bool operator!=(const ErrorResponse& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const ErrorResponse& dto);

public:
    /// Код ошибки HTTP
    std::optional<int64_t> code;

    /// Сообщение об ошибке
    std::optional<std::string> message;

    /// Дополнительные детали ошибки
    std::optional<std::string> details;

};

inline std::ostream& operator<<(std::ostream& os, const ErrorResponse& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto