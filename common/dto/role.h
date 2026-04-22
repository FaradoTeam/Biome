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
 * @brief Role DTO
 */
class Role final
{
public:
    using Ptr = std::shared_ptr<Role>;
    using ConstPtr = std::shared_ptr<const Role>;

public:
    Role() = default;
    explicit Role(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const Role& other) const;
    bool operator!=(const Role& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const Role& dto);

public:
    /// Уникальный идентификатор роли
    std::optional<int64_t> id;

    /// Название роли
    std::optional<std::string> caption;

    /// Описание роли
    std::optional<std::string> description;

};

inline std::ostream& operator<<(std::ostream& os, const Role& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto