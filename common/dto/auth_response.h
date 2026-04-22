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
 * @brief AuthResponse DTO
 */
class AuthResponse final
{
public:
    using Ptr = std::shared_ptr<AuthResponse>;
    using ConstPtr = std::shared_ptr<const AuthResponse>;

public:
    AuthResponse() = default;
    explicit AuthResponse(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const AuthResponse& other) const;
    bool operator!=(const AuthResponse& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const AuthResponse& dto);

public:
    std::optional<std::string> accessToken;

    static inline const std::string tokenType = "Bearer";

};

inline std::ostream& operator<<(std::ostream& os, const AuthResponse& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto