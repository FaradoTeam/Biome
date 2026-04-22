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
 * @brief AuthRequest DTO
 */
class AuthRequest final
{
public:
    using Ptr = std::shared_ptr<AuthRequest>;
    using ConstPtr = std::shared_ptr<const AuthRequest>;

public:
    AuthRequest() = default;
    explicit AuthRequest(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const AuthRequest& other) const;
    bool operator!=(const AuthRequest& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const AuthRequest& dto);

public:
    std::optional<std::string> login;

    std::optional<std::string> password;

};

inline std::ostream& operator<<(std::ostream& os, const AuthRequest& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto