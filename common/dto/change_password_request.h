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
 * @brief ChangePasswordRequest DTO
 */
class ChangePasswordRequest final
{
public:
    using Ptr = std::shared_ptr<ChangePasswordRequest>;
    using ConstPtr = std::shared_ptr<const ChangePasswordRequest>;

public:
    ChangePasswordRequest() = default;
    explicit ChangePasswordRequest(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const ChangePasswordRequest& other) const;
    bool operator!=(const ChangePasswordRequest& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const ChangePasswordRequest& dto);

public:
    std::optional<std::string> oldPassword;

    std::optional<std::string> newPassword;

};

inline std::ostream& operator<<(std::ostream& os, const ChangePasswordRequest& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto